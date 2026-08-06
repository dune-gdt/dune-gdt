#!/usr/bin/env python3
"""Convert the MyST-NB notebook sources in ``docs/source`` into standalone Python scripts.

The documentation notebooks (``docs/source/*.md`` in jupytext MyST format) are executed as part of
the Sphinx build, which doubles as an integration test of the Python bindings. That test is a poor
one to debug: myst-nb runs every notebook in one long ``sphinx-build`` invocation inside a Jupyter
kernel, so a kernel that dies from a signal (segfault, abort, OOM kill) surfaces as nothing but
``nbclient.exceptions.DeadKernelError: Kernel died`` -- no traceback, no indication of which cell
was running, and no way to re-run just the offending notebook without a full docs build.

This module extracts the code cells of each notebook into a plain ``.py`` script that

* runs in its own process, under a plain interpreter, with no Jupyter kernel in between,
* enables :mod:`faulthandler`, so a deadly signal dumps a C-level traceback,
* prints a marker naming the notebook, cell index and source line before every cell, so the last
  line on stderr identifies the cell that was running when the process died.

The scripts are generated (never committed) by the ``notebook_scripts`` CMake target and registered
as one CTest test per notebook, so ``ctest -L notebook`` reproduces the docs-build notebook
execution with per-notebook isolation, timeouts and exit statuses.

Usage::

    python docs/notebooks_to_scripts.py --output-dir build/notebook_scripts
    python docs/notebooks_to_scripts.py --list           # notebook names, one per line
    python docs/notebooks_to_scripts.py --list-paths     # notebook source paths, one per line
"""

from __future__ import annotations

import argparse
import re
import sys
from dataclasses import dataclass
from pathlib import Path

DEFAULT_SOURCE_DIR = Path(__file__).resolve().parent / "source"

#: opening fence of a MyST-NB code cell, e.g. ```{code-cell} or ```{code-cell} ipython3
_CELL_OPEN_RE = re.compile(
    r"^(?P<fence>`{3,}|~{3,})\{code-cell\}\s*(?P<language>\S+)?\s*$"
)
#: a directive option line directly below the opening fence, e.g. ``:tags: [remove-cell]``
_OPTION_RE = re.compile(r"^:(?P<name>[\w-]+):\s*(?P<value>.*)$")
#: an IPython line magic, e.g. ``%load_ext wurlitzer``
_LINE_MAGIC_RE = re.compile(r"^(?P<indent>\s*)%(?P<name>\w+)(?P<rest>.*)$")
#: an IPython cell magic, e.g. ``%%time``
_CELL_MAGIC_RE = re.compile(r"^\s*%%(?P<name>\w+)")
#: an IPython shell escape, e.g. ``!ls -l f.vtu``
_SHELL_RE = re.compile(r"^(?P<indent>\s*)!(?P<command>.+)$")

#: line magics that only affect how output is rendered inside a notebook frontend and that carry no
#: meaning for a plain script: dropped (as a comment) rather than translated.
_COSMETIC_MAGICS = frozenset({"load_ext", "matplotlib", "config", "gui", "pylab"})
#: line magics that prefix an ordinary statement; the statement is kept, the magic dropped.
_STATEMENT_PREFIX_MAGICS = frozenset({"time", "timeit", "prun", "capture"})


class ConversionError(RuntimeError):
    """Raised for notebook content this converter cannot faithfully translate."""


@dataclass(frozen=True)
class Cell:
    """A single extracted code cell."""

    #: 1-based index among the code cells of the notebook
    index: int
    #: 1-based line number of the opening fence in the notebook source
    line: int
    #: the cell body, already stripped of directive options
    source: str
    #: the ``:load:`` target the body was read from, if any
    loaded_from: str | None = None


def _read_front_matter(lines: list[str]) -> tuple[dict[str, str], int]:
    """Return the (flat) YAML front matter and the index of the first line after it.

    Only the top-level ``key: value`` pairs are needed here (to tell a notebook from an ordinary
    MyST page), so this deliberately avoids a YAML dependency.
    """
    if not lines or lines[0].rstrip() != "---":
        return {}, 0
    for end, line in enumerate(lines[1:], start=1):
        if line.rstrip() == "---":
            break
    else:
        raise ConversionError("unterminated YAML front matter")
    front_matter = {}
    for line in lines[1:end]:
        if line.startswith((" ", "\t")) or ":" not in line:
            continue  # nested mapping entry; the top-level keys are all we look at
        key, _, value = line.partition(":")
        front_matter[key.strip()] = value.strip()
    return front_matter, end + 1


def is_notebook(path: Path) -> bool:
    """Whether ``path`` is a MyST-NB notebook (as opposed to an ordinary MyST page)."""
    lines = path.read_text(encoding="utf-8").splitlines()
    front_matter, _ = _read_front_matter(lines)
    if "jupytext" not in front_matter and "kernelspec" not in front_matter:
        return False
    return any(_CELL_OPEN_RE.match(line) for line in lines)


def notebook_paths(source_dir: Path = DEFAULT_SOURCE_DIR) -> list[Path]:
    """All notebook sources in ``source_dir``, sorted by name."""
    return sorted(path for path in source_dir.glob("*.md") if is_notebook(path))


def extract_cells(path: Path, source_dir: Path | None = None) -> list[Cell]:
    """Extract the code cells of the notebook at ``path``.

    ``:load:`` cells (used to pull the shared ``myst_code_init.py`` preamble into every notebook)
    have their referenced file inlined, exactly as myst-nb would execute it.
    """
    source_dir = source_dir or path.parent
    lines = path.read_text(encoding="utf-8").splitlines()
    _, start = _read_front_matter(lines)

    cells: list[Cell] = []
    position = start
    while position < len(lines):
        opening = _CELL_OPEN_RE.match(lines[position])
        if not opening:
            position += 1
            continue
        fence = opening.group("fence")
        open_line = (
            position + 1
        )  # 1-based, for the marker printed by the generated script
        position += 1

        # directive options, either as ``:key: value`` lines or as a YAML block delimited by ---
        options: dict[str, str] = {}
        if position < len(lines) and lines[position].rstrip() == "---":
            position += 1
            while position < len(lines) and lines[position].rstrip() != "---":
                key, separator, value = lines[position].partition(":")
                if separator:
                    options[key.strip()] = value.strip()
                position += 1
            position += 1  # skip the closing ---
        else:
            while position < len(lines) and (
                option := _OPTION_RE.match(lines[position])
            ):
                options[option.group("name")] = option.group("value")
                position += 1

        body: list[str] = []
        while position < len(lines) and lines[position].rstrip() != fence:
            body.append(lines[position])
            position += 1
        if position >= len(lines):
            raise ConversionError(f"{path.name}:{open_line}: unterminated code cell")
        position += 1  # skip the closing fence

        loaded_from = options.get("load")
        if loaded_from:
            loaded = (source_dir / loaded_from).resolve()
            if not loaded.is_file():
                raise ConversionError(
                    f"{path.name}:{open_line}: ':load: {loaded_from}' not found"
                )
            body = loaded.read_text(encoding="utf-8").splitlines()

        source = "\n".join(body).strip("\n")
        if not source.strip():
            continue
        cells.append(
            Cell(
                index=len(cells) + 1,
                line=open_line,
                source=source,
                loaded_from=loaded_from,
            )
        )
    return cells


def translate_cell_source(source: str, where: str) -> str:
    """Translate IPython-only syntax in a cell body into plain Python.

    Anything that cannot be translated faithfully raises :class:`ConversionError` rather than being
    silently dropped -- a converter that quietly changes what a notebook does would make the
    generated tests worthless.
    """
    translated = []
    for offset, line in enumerate(source.splitlines()):
        location = f"{where}+{offset}"
        cell_magic = _CELL_MAGIC_RE.match(line)
        if cell_magic:
            raise ConversionError(
                f"{location}: cannot translate cell magic %%{cell_magic['name']}"
            )
        magic = _LINE_MAGIC_RE.match(line)
        if magic:
            name, indent, rest = magic["name"], magic["indent"], magic["rest"].strip()
            if name in _COSMETIC_MAGICS:
                translated.append(
                    f"{indent}# [notebook-script] dropped frontend magic: %{name} {rest}".rstrip()
                )
                continue
            if name in _STATEMENT_PREFIX_MAGICS:
                translated.append(
                    f"{indent}# [notebook-script] dropped magic prefix: %{name}"
                )
                if rest:
                    translated.append(f"{indent}{rest}")
                continue
            raise ConversionError(f"{location}: cannot translate line magic %{name}")
        shell = _SHELL_RE.match(line)
        if shell:
            command = shell["command"].strip().replace("\\", "\\\\").replace('"', '\\"')
            translated.append(f'{shell["indent"]}_shell("{command}")')
            continue
        translated.append(line)
    return "\n".join(translated)


_PREAMBLE = '''\
#!/usr/bin/env python3
# ruff: noqa
# fmt: off
"""Executable transcript of {notebook}.

GENERATED FILE -- DO NOT EDIT. Regenerate with

    python docs/notebooks_to_scripts.py --output-dir <dir>

or by building the ``notebook_scripts`` CMake target. Edit {notebook} instead.

Every code cell of the notebook is reproduced verbatim below, in order, preceded by a marker naming
the notebook line it came from. Run this directly to reproduce a docs-build notebook failure
outside of Sphinx/myst-nb; if the process dies from a signal, the faulthandler traceback and the
last marker on stderr say where.
"""

import builtins
import faulthandler
import os
import subprocess
import sys

faulthandler.enable()
# the notebooks visualize; keep them headless-safe when no display is around
os.environ.setdefault("MPLBACKEND", "Agg")

# IPython injects ``display`` into builtins for every notebook cell, and the k3d/ipywidgets
# rendering behind dune's visualize_function relies on that global existing. Outside IPython it
# does not, so put it back: without a running shell IPython's display() degrades to printing the
# object's repr, which is all a headless test needs.
try:
    from IPython.display import display as _display
except ImportError:  # pragma: no cover - IPython is a docs dependency, but do not hard-require it
    pass
else:
    if not hasattr(builtins, "display"):
        builtins.display = _display

_NOTEBOOK = "{notebook}"
_CELL_COUNT = {cell_count}

# myst-nb executes the notebooks with docs/source as the working directory, so several of them
# import helper modules sitting next to them (discretize_elliptic_cg, ...). The generated scripts
# run in a scratch directory instead -- keep those imports resolvable by putting the notebook's own
# directory on sys.path, exactly as docs/source/conf.py does for the Sphinx build.
_SOURCE_DIR = os.environ.get("DUNE_GDT_NOTEBOOK_SOURCE_DIR") or r"{source_dir}"
if _SOURCE_DIR and _SOURCE_DIR not in sys.path:
    sys.path.insert(0, _SOURCE_DIR)

# The notebooks write their output files (f.vtu, L_shaped_domain.msh, ...) into the working
# directory. Left unset, that is wherever this script was started from; the CTest tests point it at
# a scratch directory per notebook so parallel runs cannot overwrite each other.
_WORKDIR = os.environ.get("DUNE_GDT_NOTEBOOK_WORKDIR")
if _WORKDIR:
    os.makedirs(_WORKDIR, exist_ok=True)
    os.chdir(_WORKDIR)


def _cell(index, line):
    """Announce the cell that is about to run; the last one printed is the one that died."""
    print(
        f"[notebook-script] {{_NOTEBOOK}}:{{line}} cell {{index}}/{{_CELL_COUNT}}",
        file=sys.stderr,
        flush=True,
    )
    sys.stdout.flush()


def _shell(command):
    """Stand-in for a notebook ``!command`` shell escape."""
    print(f"$ {{command}}", flush=True)
    subprocess.run(command, shell=True, check=True)


'''

_EPILOGUE = """
print(
    f"[notebook-script] {_NOTEBOOK}: all {_CELL_COUNT} cells completed", file=sys.stderr, flush=True
)
"""


def convert_notebook(path: Path, source_dir: Path | None = None) -> str:
    """Return the standalone script transcribing the notebook at ``path``."""
    source_dir = source_dir or path.parent
    cells = extract_cells(path, source_dir)
    if not cells:
        raise ConversionError(f"{path.name}: no code cells found")

    chunks = [
        _PREAMBLE.format(
            notebook=path.name,
            cell_count=len(cells),
            source_dir=source_dir.resolve().as_posix(),
        )
    ]
    for cell in cells:
        origin = f"{path.name}:{cell.line}"
        if cell.loaded_from:
            origin += f" (:load: {cell.loaded_from})"
        chunks.append(
            f"# ---------------- cell {cell.index}/{len(cells)}, {origin} ----------------\n"
        )
        chunks.append(f"_cell({cell.index}, {cell.line})\n")
        chunks.append(translate_cell_source(cell.source, f"{path.name}:{cell.line}"))
        chunks.append("\n\n")
    chunks.append(_EPILOGUE)
    return "".join(chunks)


def write_scripts(source_dir: Path, output_dir: Path) -> list[Path]:
    """Convert every notebook in ``source_dir`` and write the scripts into ``output_dir``."""
    output_dir.mkdir(parents=True, exist_ok=True)
    written = []
    for notebook in notebook_paths(source_dir):
        script = output_dir / (notebook.stem + ".py")
        content = convert_notebook(notebook, source_dir)
        # only touch the file when the content actually changed, so a build system watching the
        # generated scripts does not re-run the (expensive) tests on every configure
        if not script.is_file() or script.read_text(encoding="utf-8") != content:
            script.write_text(content, encoding="utf-8")
        written.append(script)
    return written


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument(
        "--source-dir",
        type=Path,
        default=DEFAULT_SOURCE_DIR,
        help="directory holding the MyST-NB notebook sources (default: docs/source)",
    )
    parser.add_argument(
        "--output-dir", type=Path, help="where to write the generated scripts"
    )
    parser.add_argument(
        "--list",
        action="store_true",
        help="print the notebook names (script stems), one per line",
    )
    parser.add_argument(
        "--list-paths",
        action="store_true",
        help="print the notebook source paths, one per line",
    )
    args = parser.parse_args(argv)

    notebooks = notebook_paths(args.source_dir)
    if args.list:
        print("\n".join(notebook.stem for notebook in notebooks))
        return 0
    if args.list_paths:
        print("\n".join(str(notebook) for notebook in notebooks))
        return 0
    if not args.output_dir:
        parser.error("one of --output-dir, --list or --list-paths is required")

    for script in write_scripts(args.source_dir, args.output_dir):
        print(script)
    return 0


if __name__ == "__main__":
    sys.exit(main())

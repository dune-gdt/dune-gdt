#!/usr/bin/env python3
"""Convert the MyST-NB notebook sources in ``docs/source`` into standalone Python scripts.

The documentation notebooks (``docs/source/*.md`` in jupytext MyST format) are executed as part of
the Sphinx build, which doubles as an integration test of the Python bindings. That test is a poor
one to debug: myst-nb runs every notebook in one long ``sphinx-build`` invocation inside a Jupyter
kernel, so a kernel that dies from a signal (segfault, abort, OOM kill) surfaces as nothing but
``nbclient.exceptions.DeadKernelError: Kernel died`` -- no traceback, no indication of which cell
was running, and no way to re-run just the offending notebook without a full docs build.

The notebooks are parsed with :mod:`jupytext`, whose own format they declare in their front matter;
this module extracts the code cells of each notebook into a plain ``.py`` script that

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
from dataclasses import dataclass
from pathlib import Path

DEFAULT_SOURCE_DIR = Path(__file__).resolve().parent / "source"

#: the notebooks declare ``jupytext: text_representation: format_name: myst`` in their own front
#: matter, so jupytext parses them natively -- fences, directive options in either the ``:key:`` or
#: the ``---`` block form, and cell metadata. Only the code cells' *line numbers* have to be
#: recovered separately, since nbformat cells do not carry their source position.
_MYST_FMT = "md:myst"

#: opening fence of a MyST-NB code cell, e.g. ```{code-cell} or ```{code-cell} ipython3. Used only
#: to number the cells jupytext returns, never to parse them.
_CELL_OPEN_RE = re.compile(r"^(?:`{3,}|~{3,})\{code-cell\}")
#: an IPython line magic, e.g. ``%load_ext wurlitzer``; the argument tail starts with a non-word
#: character so the name's \w+ has nothing to backtrack into, and is absent for a bare ``%magic``
_LINE_MAGIC_RE = re.compile(r"^(?P<indent>[ \t]*)%(?P<name>\w+)(?P<rest>\W.*)?$")
#: an IPython cell magic, e.g. ``%%time``
_CELL_MAGIC_RE = re.compile(r"^[ \t]*%%(?P<name>\w+)")
#: an IPython shell escape, e.g. ``!ls -l f.vtu``
_SHELL_RE = re.compile(r"^(?P<indent>[ \t]*)!(?P<command>.+)$")
#: IPython object introspection: ``?name``/``??name`` and ``name?``/``name??``. Deliberately narrow
#: -- only a bare (dotted) name carrying the ? -- so an ordinary string with a question mark in it
#: ("what?") is never mistaken for one.
_HELP_RE = re.compile(
    r"^[ \t]*(?:\?{1,2}[\w.]+|[A-Za-z_][\w.]*(?:\(\))?\?{1,2})[ \t]*$",
)

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


def _read(path: Path):
    """Parse a MyST notebook with jupytext, as an nbformat notebook.

    Imported lazily: CMake lists the notebooks with the bare interpreter it found, at configure
    time, before any environment carrying jupytext exists. Only the conversion itself -- which runs
    through ``uv run --group notebooks`` -- needs the library.
    """
    # the missing-jupytext path is exercised by the CMake wiring, not by pytest
    try:
        import jupytext
    except ImportError as error:  # pragma: no cover
        raise ConversionError(
            f"{path.name}: converting notebooks needs jupytext; run this through"
            " `uv run --frozen --group notebooks`"
        ) from error
    try:
        return jupytext.read(path, fmt=_MYST_FMT)
    except Exception as error:  # jupytext raises assorted parser/YAML errors
        raise ConversionError(f"{path.name}: {error}") from error


def _code_cell_lines(path: Path) -> list[int]:
    """1-based source line of every ``{code-cell}`` fence in ``path``, in order.

    nbformat cells carry no source position, but the markers in the generated scripts are only
    useful if they name a line in the notebook. The Nth code cell jupytext returns is the Nth
    ``{code-cell}`` fence in the file, so numbering them is a scan rather than a parse.
    """
    return [
        number
        for number, line in enumerate(
            path.read_text(encoding="utf-8").splitlines(), start=1
        )
        if _CELL_OPEN_RE.match(line)
    ]


def is_notebook(path: Path) -> bool:
    """Whether ``path`` is a MyST-NB notebook (as opposed to an ordinary MyST page).

    A notebook declares a kernel to execute its cells in and has at least one cell to execute.
    This is deliberately a scan and not a parse: CMake calls it at configure time with the bare
    interpreter it found, where jupytext is not importable, and telling a notebook apart from a
    prose page needs no understanding of the cells themselves.
    """
    lines = path.read_text(encoding="utf-8").splitlines()
    if not lines or lines[0].rstrip() != "---":
        return False
    try:
        end = lines.index("---", 1)
    except ValueError:
        return False
    if not any(line.startswith("kernelspec:") for line in lines[1:end]):
        return False
    return any(_CELL_OPEN_RE.match(line) for line in lines[end + 1 :])


def notebook_paths(source_dir: Path = DEFAULT_SOURCE_DIR) -> list[Path]:
    """All notebook sources in ``source_dir``, sorted by name."""
    return sorted(path for path in source_dir.glob("*.md") if is_notebook(path))


def _load_body(source_dir: Path, target: str) -> str:
    """Read the body a ``:load:`` option points at, as myst-nb would."""
    loaded = (source_dir / target).resolve()
    if not loaded.is_file():
        raise ConversionError(f"':load: {target}' not found")
    return loaded.read_text(encoding="utf-8")


def extract_cells(path: Path, source_dir: Path | None = None) -> list[Cell]:
    """Extract the code cells of the notebook at ``path``.

    ``:load:`` cells (used to pull the shared ``myst_code_init.py`` preamble into every notebook)
    arrive from jupytext as an empty cell carrying a ``load`` metadata key; their referenced file is
    inlined here, exactly as myst-nb would execute it.
    """
    source_dir = source_dir or path.parent
    notebook = _read(path)
    code_cells = [cell for cell in notebook.cells if cell.cell_type == "code"]
    lines = _code_cell_lines(path)
    if len(lines) != len(code_cells):
        # a {code-cell} fence shown inside a markdown example would land here; refusing is safer
        # than silently pairing cells with the wrong line numbers
        raise ConversionError(
            f"{path.name}: found {len(lines)} code-cell fences but jupytext parsed "
            f"{len(code_cells)} code cells"
        )

    cells: list[Cell] = []
    for cell, line in zip(code_cells, lines):
        loaded_from = cell.metadata.get("load")
        try:
            source = _load_body(source_dir, loaded_from) if loaded_from else cell.source
        except ConversionError as error:
            raise ConversionError(f"{path.name}:{line}: {error}") from error
        source = source.strip("\n")
        if not source.strip():
            continue
        cells.append(
            Cell(
                index=len(cells) + 1,
                line=line,
                source=source,
                loaded_from=loaded_from,
            )
        )
    return cells


def _translate_magic(match: re.Match[str]) -> list[str]:
    """Turn a line magic into the plain-Python lines standing in for it."""
    name, indent, rest = match["name"], match["indent"], (match["rest"] or "").strip()
    if name in _COSMETIC_MAGICS:
        return [
            f"{indent}# [notebook-script] dropped frontend magic: %{name} {rest}".rstrip()
        ]
    if name in _STATEMENT_PREFIX_MAGICS:
        lines = [f"{indent}# [notebook-script] dropped magic prefix: %{name}"]
        if rest:
            lines.append(f"{indent}{rest}")
        return lines
    raise ConversionError(f"cannot translate line magic %{name}")


def _translate_line(line: str) -> list[str]:
    """Translate one cell line, raising for IPython syntax with no faithful plain-Python form."""
    cell_magic = _CELL_MAGIC_RE.match(line)
    if cell_magic:
        raise ConversionError(f"cannot translate cell magic %%{cell_magic['name']}")
    if _HELP_RE.match(line):
        # valid IPython, invalid Python. Passing it through would produce a script that fails to
        # compile with a bare SyntaxError, far from the notebook line that caused it.
        raise ConversionError(f"cannot translate IPython help syntax {line.strip()!r}")
    magic = _LINE_MAGIC_RE.match(line)
    if magic:
        return _translate_magic(magic)
    shell = _SHELL_RE.match(line)
    if shell:
        command = shell["command"].strip().replace("\\", "\\\\").replace('"', '\\"')
        return [f'{shell["indent"]}_shell("{command}")']
    return [line]


def translate_cell_source(source: str, where: str) -> str:
    """Translate IPython-only syntax in a cell body into plain Python.

    Anything that cannot be translated faithfully raises :class:`ConversionError` rather than being
    silently dropped -- a converter that quietly changes what a notebook does would make the
    generated tests worthless.
    """
    translated: list[str] = []
    # 1-based, like every other line number this tool reports: "+1" is the cell's first line
    for offset, line in enumerate(source.splitlines(), start=1):
        try:
            translated.extend(_translate_line(line))
        except ConversionError as error:
            raise ConversionError(f"{where}+{offset}: {error}") from error
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
import resource
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

# pyMOR chooses its visualisation backend from whether it is running inside Jupyter
# (pymor.core.config.is_jupyter): the notebooks are, these scripts are not. Left alone, fom.visualize()
# resolves "jupyter_or_gl" to the Qt backend and dies with QtMissingError, and PatchVisualizer imports
# Qt on every branch except the jupyter one. docs/source/myst_code_init.py loads pyMOR's jupyter
# extension for exactly this reason, but only `if get_ipython() is not None`. Select the same backend
# explicitly through pyMOR's own defaults mechanism, so the rendering code still runs rather than
# being stubbed out. The module has to be imported first: that is what registers the defaults keys.
try:
    import pymor.discretizers.builtin.gui.visualizers  # noqa: F401
    from pymor.core.defaults import set_defaults as _set_defaults
except ImportError:  # pragma: no cover - pyMOR is a notebook dependency, not a hard requirement
    pass
else:
    _set_defaults(
        {{
            "pymor.discretizers.builtin.gui.visualizers.PatchVisualizer.backend": "jupyter",
            "pymor.discretizers.builtin.gui.visualizers.OnedVisualizer.backend": "jupyter",
        }}
    )

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


def _peak_rss_mb():
    """Peak resident set size so far, in MiB (ru_maxrss is in KiB on Linux)."""
    return resource.getrusage(resource.RUSAGE_SELF).ru_maxrss / 1024


def _cell(index, line):
    """Announce the cell that is about to run; the last one printed is the one that died.

    The running peak RSS goes out with it, so a process that is killed outright still leaves a
    memory trace up to its last surviving line -- which is the only evidence an OOM kill leaves
    inside the process.
    """
    # Flush stdout BEFORE writing the marker, never after. stdout is a pipe under CTest/CI and so
    # is block-buffered, while the marker goes to unbuffered stderr: flushing afterwards lets the
    # previous cell's output surface below this marker, which reads as output of the cell that is
    # only about to start. That mis-attribution is exactly what a crash report must not do.
    sys.stdout.flush()
    print(
        f"[notebook-script] {{_NOTEBOOK}}:{{line}} cell {{index}}/{{_CELL_COUNT}}"
        f" (peak RSS {{_peak_rss_mb():.0f}} MiB)",
        file=sys.stderr,
        flush=True,
    )


def _shell(command):
    """Stand-in for a notebook ``!command`` shell escape."""
    print(f"$ {{command}}", flush=True)
    subprocess.run(command, shell=True, check=True)


'''

_EPILOGUE = """
print(
    f"[notebook-script] {_NOTEBOOK}: all {_CELL_COUNT} cells completed"
    f" (peak RSS {_peak_rss_mb():.0f} MiB)",
    file=sys.stderr,
    flush=True,
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
    output_dir = output_dir.resolve()
    output_dir.mkdir(parents=True, exist_ok=True)
    written = []
    for notebook in notebook_paths(source_dir):
        script = (output_dir / (notebook.stem + ".py")).resolve()
        # the script name is derived from a notebook file name, so it cannot escape output_dir --
        # but this is the only place this tool writes, so make that a checked property rather than
        # an assumption about what a future --source-dir can contain
        if script.parent != output_dir:
            raise ConversionError(f"{notebook.name}: would write outside {output_dir}")
        content = convert_notebook(notebook, source_dir)
        # only touch the file when the content actually changed, so a build system watching the
        # generated scripts does not re-run the (expensive) tests on every configure
        if not script.is_file() or script.read_text(encoding="utf-8") != content:
            script.write_text(content, encoding="utf-8")
        written.append(script)
    return written


def main(argv: list[str] | None = None) -> None:
    """Run the converter's CLI; a ConversionError propagates and fails the caller."""
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
    elif args.list_paths:
        print("\n".join(str(notebook) for notebook in notebooks))
    elif not args.output_dir:
        parser.error("one of --output-dir, --list or --list-paths is required")
    else:
        for script in write_scripts(args.source_dir, args.output_dir):
            print(script)


if __name__ == "__main__":
    main()

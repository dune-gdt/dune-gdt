"""Execute the MyST documentation notebooks (source/*.md) as a pytest suite.

The notebooks are the only end-to-end exercise of the Python bindings against a realistic
workflow (grid -> space -> assembly -> solve -> visualize). Until now they only ran inside the
Sphinx build (myst-nb, see source/conf.py), which takes ~31 min and reports failures as
nbformat error output. Here each notebook is converted on the fly by jupytext into a
``py:percent`` script and exec'd in-process, so a failure is an ordinary traceback into a real
file on disk: ``--pdb``, ``-x`` and ``--lf`` all work, and the notebook code is visible to
``coverage``.

CMake registers one CTest test per notebook (see DXT_ADD_PYTHON_TESTS in
cmake/modules/DuneXTTesting.cmake), selecting it by the parametrize id, which is the file stem.
"""

import builtins
import re
from pathlib import Path

import pytest

SOURCE_DIR = Path(__file__).resolve().parent / "source"

# The magics and shell escapes in these notebooks are display chrome, not part of what is being
# tested, and they are syntax errors in a plain script. The full inventory across all notebooks is
# `%load_ext wurlitzer` (13x) -- it forwards dune's C-level stdout into the notebook, which for a
# script already goes to the terminal -- and `!ls -l *.vtu` (2x), which just shows that visualize()
# wrote a file. Dropping both is a no-op for the code under test.
#
# Deliberately a line filter and not a parser: it would also blank a line starting with % or ! inside
# a triple-quoted string. No notebook has one, and a real tokenizer here would be more machinery than
# the 15 lines it guards. If that ever bites, the fix is to strip per code-cell off the notebook
# object instead of off the serialised script.
_MAGIC_LINE = re.compile(r"(?m)^[ \t]*[%!].*$")


def _notebooks():
    """The source/*.md files that are notebooks; index.md, examples.md, tutorials.md and
    benchmarks.md are ordinary prose pages with no code cells and no jupytext front matter."""
    return sorted(
        p for p in SOURCE_DIR.glob("*.md") if "jupytext" in p.read_text()[:512]
    )


@pytest.fixture(autouse=True)
def _kernel_equivalents(monkeypatch):
    """Supply the two things a Jupyter kernel provides that a plain script does not.

    Both stand in for the `:load: myst_code_init.py` init cell, which every notebook carries and
    which jupytext renders as an empty cell -- and which is a no-op outside IPython anyway, since
    its whole body is guarded on `get_ipython() is not None`.

    `display`: see python/xt/test/conftest.py::_k3d_display_builtin -- dune.xt.common.vtk.plot
    calls a bare `display(...)` (via k3d's Plot.display()), which a frontend installs into builtins
    but ctest does not. 13 notebooks call visualize().

    See also _headless_pymor_visualizers below for the third one, which is applied only to the
    notebooks that need it.
    """
    import IPython.display

    monkeypatch.setattr(builtins, "display", IPython.display.display, raising=False)


def _headless_pymor_visualizers():
    """Point pymor's builtin visualizers at a backend that does not need a GUI toolkit.

    Both visualizer classes choose their backend with `is_jupyter()` (pymor/core/config.py) and,
    off-kernel, fall back to `gl` -- so example__gmsh_grid's `fom.visualize(fom.solve())` dies with
    `QtMissingError: cannot visualize: import of Qt bindings failed`. Pinning them to `jupyter` is
    what the kernel picks in the myst-nb docs build, so the notebook renders the same way here.

    It also has to be `jupyter` specifically: PatchVisualizer.visualize only skips Qt on that one
    branch, and routes every other backend -- `matplotlib` included -- through
    pymor.discretizers.builtin.gui.qt, so `matplotlib` means matplotlib inside a Qt window and
    fails identically.

    Setting the `backend` default is pymor's own supported knob (both __init__s are decorated with
    `@defaults('backend')`), so this configures pymor rather than patching it. The tempting
    alternative, PYMOR_FORCE_JUPYTER, does not work: it makes pymor/__init__.py call
    `get_ipython().run_line_magic(...)` at import time, and that is None in a plain script.
    """
    from pymor.core.defaults import set_defaults

    prefix = "pymor.discretizers.builtin.gui.visualizers"
    set_defaults(
        {
            f"{prefix}.PatchVisualizer.backend": "jupyter",
            f"{prefix}.OnedVisualizer.backend": "jupyter",
        }
    )


@pytest.mark.parametrize("notebook", _notebooks(), ids=lambda p: p.stem)
def test_notebook(notebook, tmp_path, monkeypatch):
    jupytext = pytest.importorskip("jupytext")
    source = _MAGIC_LINE.sub(
        "", jupytext.writes(jupytext.read(notebook), fmt="py:percent")
    )
    # written out so tracebacks resolve to real source lines, and so a failing notebook leaves the
    # exact script that failed in the pytest tmp dir for reproduction
    if "pymor" in source:
        _headless_pymor_visualizers()
    script = tmp_path / f"{notebook.stem}.py"
    script.write_text(source)
    # notebooks write (and read back) .vtu/.msh files under the cwd -- keep that out of the source
    # tree. The helper modules they import (discretize_elliptic_cg/_ipdg) live next to them, which
    # in a kernel is covered by the notebook's own directory being the cwd.
    monkeypatch.chdir(tmp_path)
    monkeypatch.syspath_prepend(str(SOURCE_DIR))
    # executing the notebook *is* the test here
    exec(
        compile(source, str(script), "exec"),
        {"__name__": "__main__", "__file__": str(script)},
    )

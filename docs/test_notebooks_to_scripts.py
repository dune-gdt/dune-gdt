"""Unit tests for the notebook-to-script converter (see notebooks_to_scripts.py).

These check the translation itself -- cell extraction, ``:load:`` inlining, IPython syntax -- without
executing a notebook, so they run fast and without the dune-gdt bindings. They also assert against the
real docs/source notebooks, so a notebook that starts using syntax the converter cannot translate
fails here rather than silently dropping a cell out of the generated tests.
"""

import sys
from pathlib import Path

import pytest

DOCS_DIR = Path(__file__).resolve().parent
sys.path.insert(0, str(DOCS_DIR))

import notebooks_to_scripts as nb2s  # noqa: E402

FRONT_MATTER = """\
---
jupytext:
  text_representation:
    format_name: myst
kernelspec:
  display_name: Python 3
  name: python3
---
"""


def _write(directory: Path, name: str, body: str) -> Path:
    path = directory / name
    path.write_text(FRONT_MATTER + body, encoding="utf-8")
    return path


def test_extracts_cells_in_order(tmp_path):
    notebook = _write(
        tmp_path,
        "nb.md",
        """
Some prose that is not a cell.

```{code-cell}
first = 1
```

```{code-cell}
second = first + 1
```
""",
    )
    cells = nb2s.extract_cells(notebook)
    assert [cell.source for cell in cells] == ["first = 1", "second = first + 1"]
    assert [cell.index for cell in cells] == [1, 2]
    # the recorded line numbers point at the opening fence in the notebook
    lines = notebook.read_text(encoding="utf-8").splitlines()
    for cell in cells:
        assert lines[cell.line - 1].startswith("```{code-cell}")


def test_ignores_non_code_directives(tmp_path):
    notebook = _write(
        tmp_path,
        "nb.md",
        """
```{try_on_binder}
```

```{admonition} Not code
Neither is this.
```

```{code-cell}
only_this = True
```
""",
    )
    assert [cell.source for cell in nb2s.extract_cells(notebook)] == [
        "only_this = True"
    ]


def test_directive_options_are_stripped_and_load_is_inlined(tmp_path):
    (tmp_path / "init.py").write_text("preamble = True\n", encoding="utf-8")
    notebook = _write(
        tmp_path,
        "nb.md",
        """
```{code-cell}
:tags: [remove-cell]
:load: init.py
```

```{code-cell}
:tags: [hide-input]
body = 1
```
""",
    )
    cells = nb2s.extract_cells(notebook)
    assert cells[0].source == "preamble = True"
    assert cells[0].loaded_from == "init.py"
    # the option line must not survive into the script -- it is not valid Python
    assert cells[1].source == "body = 1"


def test_yaml_style_options_are_stripped(tmp_path):
    notebook = _write(
        tmp_path,
        "nb.md",
        """
```{code-cell}
---
tags: [remove-cell]
---
body = 1
```
""",
    )
    assert [cell.source for cell in nb2s.extract_cells(notebook)] == ["body = 1"]


def test_missing_load_target_is_an_error(tmp_path):
    notebook = _write(tmp_path, "nb.md", "\n```{code-cell}\n:load: nope.py\n```\n")
    with pytest.raises(nb2s.ConversionError, match="nope.py"):
        nb2s.extract_cells(notebook)


def test_a_code_cell_fence_that_jupytext_does_not_parse_is_an_error(tmp_path):
    # the fence scan supplying line numbers and jupytext's cell list have to agree; pairing cells
    # with the wrong lines would make every marker in the generated script point at the wrong place
    notebook = _write(
        tmp_path,
        "nb.md",
        "\n````{note}\n```{code-cell}\nshown, not run\n```\n````\n\n```{code-cell}\nx = 1\n```\n",
    )
    with pytest.raises(nb2s.ConversionError, match="code-cell fences"):
        nb2s.extract_cells(notebook)


def test_frontend_magics_are_dropped(tmp_path):
    translated = nb2s.translate_cell_source(
        "%load_ext wurlitzer\nimport numpy\n", "nb.md:1"
    )
    # the magic survives only as a comment recording what was dropped, never as a statement
    assert translated.splitlines()[0].startswith("# ")
    assert "%load_ext wurlitzer" in translated.splitlines()[0]
    assert "import numpy" in translated
    compile(translated, "cell.py", "exec")


def test_statement_prefix_magics_keep_their_statement(tmp_path):
    translated = nb2s.translate_cell_source("%time compute()", "nb.md:1")
    assert "compute()" in translated
    assert not any(line.strip().startswith("%") for line in translated.splitlines())


def test_argumentless_magics_are_dropped(tmp_path):
    # the magic's argument tail is optional; a bare %magic must not fall through as a statement
    translated = nb2s.translate_cell_source("%matplotlib\n  %load_ext x", "nb.md:1")
    assert translated.splitlines()[0].startswith("# ")
    # indentation of an indented magic is preserved, so it stays inside its block
    assert translated.splitlines()[1].startswith("  # ")
    compile(translated, "cell.py", "exec")


def test_shell_escapes_become_subprocess_calls(tmp_path):
    translated = nb2s.translate_cell_source("!ls -l f.vtu", "nb.md:1")
    assert translated == '_shell("ls -l f.vtu")'


def test_untranslatable_magics_are_an_error(tmp_path):
    with pytest.raises(nb2s.ConversionError, match="%%writefile"):
        nb2s.translate_cell_source("%%writefile out.txt\nhi\n", "nb.md:1")
    with pytest.raises(nb2s.ConversionError, match="%bookmark"):
        nb2s.translate_cell_source("%bookmark here", "nb.md:1")


@pytest.mark.parametrize(
    "line",
    ["?grid", "??grid", "grid?", "grid??", "dune.gdt.Operator?", "make_cube_grid()??"],
)
def test_ipython_help_syntax_is_an_error(line):
    # valid IPython, invalid Python: passing it through would only surface as a bare SyntaxError
    with pytest.raises(nb2s.ConversionError, match="IPython help syntax"):
        nb2s.translate_cell_source(line, "nb.md:1")


@pytest.mark.parametrize(
    "line",
    [
        'print("what?")',
        'x = "?"',
        "# is this right?",
        'raise ValueError("missing?")',
    ],
)
def test_question_marks_in_ordinary_python_are_left_alone(line):
    # the help detection must not fire on a question mark inside a string or comment
    assert nb2s.translate_cell_source(line, "nb.md:1") == line


def test_translation_errors_point_at_a_1_based_cell_line(tmp_path):
    # the offset is 1-based, like every other line number this tool reports
    with pytest.raises(nb2s.ConversionError, match=r"nb\.md:7\+1:"):
        nb2s.translate_cell_source("%bookmark here", "nb.md:7")
    with pytest.raises(nb2s.ConversionError, match=r"nb\.md:7\+3:"):
        nb2s.translate_cell_source("x = 1\ny = 2\n%bookmark here", "nb.md:7")


def test_generated_script_is_valid_python_and_marks_every_cell(tmp_path):
    notebook = _write(
        tmp_path,
        "nb.md",
        "\n```{code-cell}\nx = 1\n```\n\n```{code-cell}\ny = 2\n```\n",
    )
    script = nb2s.convert_notebook(notebook)
    compile(script, "nb.py", "exec")
    assert script.count("_cell(") == 3  # two announcements plus the helper's definition
    assert "faulthandler.enable()" in script
    assert (
        'r"' + tmp_path.as_posix() + '"' in script
    )  # the notebook dir lands on sys.path


def test_cell_marker_flushes_stdout_before_writing(tmp_path):
    # ordering is load-bearing: stdout is block-buffered on a pipe, the marker goes to stderr. If
    # stdout were flushed after the marker, the previous cell's output would appear under the next
    # cell's marker and a crash would be attributed to the wrong cell.
    notebook = _write(tmp_path, "nb.md", "\n```{code-cell}\nx = 1\n```\n")
    body = nb2s.convert_notebook(notebook)
    marker = body.index("def _cell(")
    flush = body.index("sys.stdout.flush()", marker)
    write = body.index("file=sys.stderr", marker)
    assert flush < write


def test_unchanged_notebooks_are_not_rewritten(tmp_path):
    # write_scripts must be idempotent down to the file mtime: the generated scripts are a build
    # input, so rewriting identical content would invalidate the stamp and re-run every notebook
    # test on each configure
    _write(tmp_path, "nb.md", "\n```{code-cell}\nx = 1\n```\n")
    output_dir = tmp_path / "out"
    (script,) = nb2s.write_scripts(tmp_path, output_dir)
    first = script.read_text(encoding="utf-8")
    mtime = script.stat().st_mtime_ns

    nb2s.write_scripts(tmp_path, output_dir)
    assert script.read_text(encoding="utf-8") == first
    assert script.stat().st_mtime_ns == mtime


def test_edited_notebooks_are_rewritten(tmp_path):
    _write(tmp_path, "nb.md", "\n```{code-cell}\nx = 1\n```\n")
    output_dir = tmp_path / "out"
    (script,) = nb2s.write_scripts(tmp_path, output_dir)
    before = script.read_text(encoding="utf-8")

    _write(tmp_path, "nb.md", "\n```{code-cell}\nx = 2\n```\n")
    nb2s.write_scripts(tmp_path, output_dir)
    assert script.read_text(encoding="utf-8") != before
    assert "x = 2" in script.read_text(encoding="utf-8")


def test_only_notebooks_are_picked_up(tmp_path):
    _write(tmp_path, "notebook.md", "\n```{code-cell}\nx = 1\n```\n")
    # front matter but no code cells, and code-cell-looking content but no front matter
    (tmp_path / "prose.md").write_text(FRONT_MATTER + "\njust text\n", encoding="utf-8")
    (tmp_path / "plain.md").write_text("```{code-cell}\nx = 1\n```\n", encoding="utf-8")
    assert [path.name for path in nb2s.notebook_paths(tmp_path)] == ["notebook.md"]


def test_write_scripts_covers_every_notebook(tmp_path):
    _write(tmp_path, "a.md", "\n```{code-cell}\nx = 1\n```\n")
    _write(tmp_path, "b.md", "\n```{code-cell}\ny = 2\n```\n")
    written = nb2s.write_scripts(tmp_path, tmp_path / "out")
    assert sorted(path.name for path in written) == ["a.py", "b.py"]


# --- the real notebooks -------------------------------------------------------------------------
# The converter is only useful if it covers docs/source in full; these tests fail the moment a
# notebook grows syntax that cannot be translated, instead of losing the cell at generation time.

REAL_NOTEBOOKS = nb2s.notebook_paths(nb2s.DEFAULT_SOURCE_DIR)


def test_the_docs_actually_contain_notebooks():
    assert len(REAL_NOTEBOOKS) > 1


@pytest.mark.parametrize("notebook", REAL_NOTEBOOKS, ids=lambda path: path.stem)
def test_every_documentation_notebook_converts_to_valid_python(notebook):
    script = nb2s.convert_notebook(notebook, nb2s.DEFAULT_SOURCE_DIR)
    compile(script, notebook.stem + ".py", "exec")
    # every code cell of the notebook has to show up in the script
    assert (
        script.count("_cell(")
        == len(nb2s.extract_cells(notebook, nb2s.DEFAULT_SOURCE_DIR)) + 1
    )

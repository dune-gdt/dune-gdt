import glob
import os
import sys
from pathlib import Path

import slugify
import sphinx

import dune.gdt

# Check Sphinx version
if sphinx.__version__ < "3.4":
    raise RuntimeError("Sphinx 3.4 or newer required")

needs_sphinx = "3.4"

# -----------------------------------------------------------------------------
# General configuration
# -----------------------------------------------------------------------------

this_dir = Path(__file__).resolve().parent
src_dir = (this_dir / ".." / ".." / "src").resolve()
sys.path.insert(0, str(src_dir))
sys.path.insert(0, str(this_dir))
# the branch being built; GitHub Actions sets GITHUB_REF_NAME. The legacy
# GitLab variable is kept as a fallback for local/legacy invocations.
branch = os.environ.get("GITHUB_REF_NAME", os.environ.get("CI_COMMIT_REF_NAME", "main"))
sys.path.insert(0, str(this_dir / "_ext" ))

# Add any Sphinx extension module names here, as strings. They can be extensions
# coming with Sphinx (named 'sphinx.ext.*') or your custom ones.
extensions = [
    "sphinx.ext.autodoc",
    "sphinx.ext.coverage",
    "sphinx.ext.autosummary",
    "sphinx.ext.linkcode",
    "sphinx.ext.intersphinx",
    "try_on_binder",
    "myst_nb",
    "sphinx.ext.mathjax",
    "sphinxcontrib.bibtex",
    # extracts the C++ API from the in-tree headers (configured below); replaces
    # the former Doxygen setup
    "clangquill.sphinx_ext",
    "benchmark_plots",
]
# this enables:
# substitutions-with-jinja2, direct-latex-math and definition-lists
# ref: https://myst-parser.readthedocs.io/en/latest/using/syntax-optional.html
myst_enable_extensions = [
    "dollarmath",
    "amsmath",
    "deflist",
    "html_image",
    "colon_fence",
    "smartquotes",
    "replacements",
    "substitution",
]
myst_url_schemes = ["http", "https", "mailto"]
# auto genereated link anchors
myst_heading_anchors = 2
import substitutions  # noqa

myst_substitutions = substitutions.myst_substitutions
nb_execution_mode = "cache"
nb_execution_timeout = 240  # there is an interpolation test
# print tracebacks to stdout
nb_execution_show_tb = True
# Do not abort on the first failing notebook: let myst-nb execute every notebook
# and emit a warning (with traceback) for each failure. The CI job fails the build
# if any executed notebook wrote an error report (see non_docker_build.yml).
nb_execution_raise_on_error = False
# The remaining notebooks still rely on dune-gdt python bindings that are currently
# commented out in the C++ sources. They are restored and rendered, but not executed,
# until those bindings are restored (tracked in #126) -- migrating the notebook bodies
# to the current two-level assembly API is not sufficient here, because the required
# functionality has no python-exposed replacement:
#
# * example__MNS2002_estimates.md: the local-indicator assembly uses the generic
#   `Operator += LocalElement/IntersectionBilinearFormIndicatorOperator`, whose
#   `__iadd__`/`append` overloads are commented out in operators/operator.cc (only the
#   `apply` methods are bound).
# * example__ESV2007_estimates.md: same generic `Operator +=` blocker, plus
#   `oswald_interpolation` and `LaplaceIpdgFluxReconstructionOperator` expose no symbols
#   (their bindings are guarded by `#if 0`).
# * example__gmsh_grid.md: needs `meshio` (and a gmsh v2 mesh) for pymor's
#   `discretize_gmsh`; the runners ship gmsh v4, which dune-grid cannot read.
# * example__ipdg_heat_equation.md: its assembly was migrated to the `BilinearForm`
#   + `append` API (and that part executes fine), but the time-stepping cells form
#   `m_h + dt*a_h`, i.e. a `GDT::LincombOperator`, whose pybind type is not registered
#   ("Unregistered type ... LincombOperator" / "Could not activate keep_alive!"). The
#   migration is kept so that only the LincombOperator registration remains to re-enable.
#
# The CG-FEM tutorial and the data-functions tutorial were re-enabled: the former by
# migrating its assembly cells to the `BilinearForm` + `append` API, the latter
# unchanged (it already relied solely on the migrated discretize_elliptic_cg helper).
#
# example__prolongations_products_and_norms.md is also executed again: the
# `BilinearForm` product/norm bindings used for `a(u, u)` have been finalized
# (`BilinearForm.apply2(u, u)` / `BilinearForm.norm(u)`) and the notebook migrated.
# Tracked in #127 (blocked on #126).
nb_execution_excludepatterns = [
    "*example__ESV2007_estimates.md",
    "*example__MNS2002_estimates.md",
    "*example__gmsh_grid.md",
    "*example__ipdg_heat_equation.md",
]

# -----------------------------------------------------------------------------
# clangquill C++ API documentation
# -----------------------------------------------------------------------------
# Parse the in-tree dune-gdt / dune-xt C++ headers with libclang and render MyST
# pages that Sphinx indexes through its C++ domain. This replaces the former
# Doxygen-based C++ API setup (the removed doc/doxygen target). The generated
# pages are written under this srcdir into clangquill_output_dir and pulled into
# the manual through the cpp_api/index toctree entry in index.md.
#
# All clangquill paths are resolved relative to this srcdir (tutorials/source),
# so "../.." is the repository root: the input glob covers dune/{gdt,xt}/**/*.hh
# and the include dir lets the intra-tree `#include <dune/...>` headers resolve.
# The external DUNE dependency headers (dune-common, dune-grid, ...) are not
# present in the docs environment; libclang reports those as non-fatal
# diagnostics and still extracts every symbol it can parse. Should the wheel be
# built without libclang, the extension degrades to a placeholder page rather
# than failing the build.


def _clang_resource_dir():
    """Locate the clang builtin-header directory (``stddef.h`` & co.).

    clangquill's bundled libclang ships no builtin headers, so any system
    ``#include`` (``<cstddef>``, ``<vector>``, ...) fails with ``'stddef.h' file
    not found`` unless we point clang at a resource directory. Prefer an explicit
    ``CLANGQUILL_CLANG_RESOURCE_DIR`` override, otherwise ask any ``clang`` on
    ``PATH`` where its builtins live. Returns ``None`` when none is found, which
    leaves clang to its own (here unset) default — generation still runs, just
    with more diagnostics.
    """
    import shutil  # noqa: PLC0415
    import subprocess  # noqa: PLC0415

    override = os.environ.get("CLANGQUILL_CLANG_RESOURCE_DIR")
    if override:
        return override
    clang = (
        shutil.which("clang") or shutil.which("clang-18") or shutil.which("clang-19")
    )
    if not clang:
        return None
    try:
        out = subprocess.run(
            [clang, "-print-resource-dir"],
            capture_output=True,
            text=True,
            check=True,
        )
    except (OSError, subprocess.SubprocessError):
        return None
    return out.stdout.strip() or None


clangquill_input = ["../../dune/**/*.hh"]
clangquill_include_dirs = ["../.."]
clangquill_std = "c++17"
clangquill_clang_resource_dir = _clang_resource_dir()
clangquill_output_dir = "cpp_api"
clangquill_group_by = "file"
# Emit only symbols that carry a documentation comment; flip to True to also
# list the (largely templated) undocumented internals.
clangquill_include_undocumented = False
# Persist the SQLite IR + page hashes so local rebuilds are incremental.
clangquill_cache_dir = "_clangquill_cache"

bibtex_bibfiles = ["bibliography.bib"]
# Add any paths that contain templates here, relative to this directory.
templates_path = ["_templates"]

# The suffix of source filenames.
source_suffix = {
    ".rst": "restructuredtext",
    ".ipynb": "myst-nb",
    ".md": "myst-nb",
}

# The master toctree document.
master_doc = "index"

# General substitutions.
project = "dune-gdt"
copyright = "2013-2021 dune-gdt developers and contributors"

rst_epilog = substitutions.substitutions

# There are two options for replacing |today|: either, you set today to some
# non-false value, then it is used:
# today = ''
# Else, today_fmt is used as the format for a strftime call.
today_fmt = "%B %d, %Y"

# List of documents that shouldn't be included in the build.
# unused_docs = []

# The reST default role (used for this markup: `text`) to use for all documents.
default_role = "literal"

# List of directories, relative to source directories, that shouldn't be searched
# for source files.
exclude_dirs = []

# If true, '()' will be appended to :func: etc. cross-reference text.
add_function_parentheses = False

# If true, the current module name will be prepended to all description
# unit titles (such as .. function::).
# add_module_names = True

# If true, sectionauthor and moduleauthor directives will be shown in the
# output. They are ignored by default.
# show_authors = False

# The name of the Pygments (syntax highlighting) style to use.
pygments_style = "default"

# -----------------------------------------------------------------------------
# HTML output
# -----------------------------------------------------------------------------

# The style sheet to use for HTML and HTML Help pages. A file of that name
# must exist either in Sphinx' static/ path, or in one of the custom paths
# given in html_static_path.

html_theme = "furo"
html_theme_options = {}
# The name for this set of Sphinx documents.  If None, it defaults to
# "<project> v<release> documentation".
version = dune.gdt.__version__
html_title = f"{project} v{version} Manual"

# The name of an image file (within the static path) to place at the top of
# the sidebar.
# html_logo = '../../logo/pymor_logo_white.svg'

# The name of an image file to use as favicon.
# html_favicon = '../../logo/pymor_favicon.png'

# Add any paths that contain custom static files (such as style sheets) here,
# relative to this directory. They are copied after the builtin static files,
# so a file named "default.css" will overwrite the builtin "default.css".
html_static_path = ["_static"]

# If not '', a 'Last updated on:' timestamp is inserted at every page bottom,
# using the given strftime format.
html_last_updated_fmt = "%b %d, %Y"

# If true, SmartyPants will be used to convert quotes and dashes to
# typographically correct entities.
# html_use_smartypants = True

# Custom sidebar templates, maps document names to template names.
# all: "**": ["logo-text.html", "globaltoc.html", "localtoc.html", "searchbox.html"]
# refer to https://github.com/pradyunsg/furo/blob/main/src/furo/theme/furo/theme.conf
# for available ones
html_sidebars = {
    "**": [
        "sidebar/brand.html",
        "sidebar/search.html",
        "sidebar/scroll-start.html",
        "sidebar/navigation.html",
        "sidebar/scroll-end.html",
        "sidebar/variant-selector.html",
    ]
}
# Additional templates that should be rendered to pages, maps page names to
# template names.
# html_additional_pages = {
#    'index': 'indexcontent.html',
# }

# If false, no module index is generated.
html_use_modindex = True

# If true, the reST sources are included in the HTML build as _sources/<name>.
# html_copy_source = True

# If true, an OpenSearch description file will be output, and all pages will
# contain a <link> tag referring to it.  The value of this option must be the
# base URL from which the finished HTML is served.
# html_use_opensearch = ''

# If nonempty, this is the file name suffix for HTML files (e.g. ".html").
# html_file_suffix = '.html'

# Hide link to page source.
html_show_sourcelink = False

# Output file base name for HTML help builder.
htmlhelp_basename = "dune-gdt"

# Pngmath should try to align formulas properly.
pngmath_use_preview = True

# -----------------------------------------------------------------------------
# LaTeX output
# -----------------------------------------------------------------------------

# The paper size ('letter' or 'a4').
# latex_paper_size = 'letter'

# The font size ('10pt', '11pt' or '12pt').
# latex_font_size = '10pt'

# Grouping the document tree into LaTeX files. List of tuples
# (source start file, target name, title, author, document class [howto/manual]).
# _stdauthor = 'Written by the NumPy community'
# latex_documents = [
#    ('reference/index', 'numpy-ref.tex', 'NumPy Reference',
#     _stdauthor, 'manual'),
#    ('user/index', 'numpy-user.tex', 'NumPy User Guide',
#     _stdauthor, 'manual'),
# ]

# The name of an image file (relative to this directory) to place at the top of
# the title page.
# latex_logo = None

# For "manual" documents, if this is true, then toplevel headings are parts,
# not chapters.
# latex_use_parts = False

# Additional stuff for the LaTeX preamble.

# Documents to append as an appendix to all manuals.
# latex_appendices = []

# If false, no module index is generated.
latex_use_modindex = False

# -----------------------------------------------------------------------------
# Autosummary
# -----------------------------------------------------------------------------

autosummary_generate = glob.glob("generated/*.rst")

# -----------------------------------------------------------------------------
# Coverage checker
# -----------------------------------------------------------------------------
coverage_ignore_modules = r"""
    """.split()
coverage_ignore_functions = r"""
    test($|_) (some|all)true bitwise_not cumproduct pkgload
    generic\.
    """.split()
coverage_ignore_classes = r"""
    """.split()

coverage_c_path = []
coverage_c_regexes = {}
coverage_ignore_c_items = {}

# autodoc_default_flags = ['members', 'undoc-members', 'show-inheritance']

# PyQt5 inventory is only used internally, actual link targets PySide2
intersphinx_mapping = {
    "python": ("https://docs.python.org/3", None),
    "numpy": ("https://numpy.org/doc/stable/", None),
    "PyQt5": ("https://www.riverbankcomputing.com/static/Docs/PyQt5", None),
    "scipy": ("https://docs.scipy.org/doc/scipy/reference", None),
    "matplotlib": ("https://matplotlib.org", None),
    "Sphinx": (" https://www.sphinx-doc.org/en/master/", None),
}

modindex_common_prefix = ["dune."]

# make intersphinx link to pyside2 docs
qt_documentation = "PySide2"

try_on_binder_branch = branch.replace("github/PUSH_", "from_fork__")
try_on_binder_slug = os.environ.get(
    "CI_COMMIT_REF_SLUG", slugify.slugify(try_on_binder_branch)
)


# repository hosting both the tutorial sources and the python bindings
_linkcode_baseurl = "https://github.com/dune-gdt/dune-gdt"
_repo_root = (this_dir / ".." / "..").resolve()


def linkcode_resolve(domain, info):
    if domain != "py" or not info["module"]:
        return None
    parts = info["module"].split(".")
    # dune.gdt / dune.xt bindings live under python/<gdt|xt>/dune/<gdt|xt>/...,
    # while the tutorial helper modules sit next to this conf.py in
    # tutorials/source/.
    if parts[:2] == ["dune", "gdt"]:
        rel = Path("python", "gdt", *parts)
    elif parts[:2] == ["dune", "xt"]:
        rel = Path("python", "xt", *parts)
    else:
        rel = Path("tutorials", "source", *parts)
    # a package resolves to its __init__.py, a plain module to <name>.py
    if (_repo_root / rel / "__init__.py").is_file():
        rel = rel / "__init__.py"
    else:
        rel = rel.with_suffix(".py")
    return f"{_linkcode_baseurl}/tree/{branch}/{rel.as_posix()}"

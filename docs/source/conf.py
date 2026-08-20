import glob
import logging
import os
import sys
from pathlib import Path

import slugify
import sphinx
import sphinx.util.logging
from sphinx.errors import ConfigError

import dune.gdt

# Check Sphinx version
if sphinx.__version__ < "3.4":
    raise RuntimeError("Sphinx 3.4 or newer required")

needs_sphinx = "3.4"

# -----------------------------------------------------------------------------
# General configuration
# -----------------------------------------------------------------------------

this_dir = Path(__file__).resolve().parent
# repository root: this file lives in docs/source (used for the C++ API parse
# below and for the linkcode source links at the bottom of this file)
_repo_root = (this_dir / ".." / "..").resolve()
src_dir = (this_dir / ".." / ".." / "src").resolve()
sys.path.insert(0, str(src_dir))
sys.path.insert(0, str(this_dir))
# the branch being built; GitHub Actions sets GITHUB_REF_NAME. The legacy
# GitLab variable is kept as a fallback for local/legacy invocations.
branch = os.environ.get("GITHUB_REF_NAME", os.environ.get("CI_COMMIT_REF_NAME", "main"))
sys.path.insert(0, str(this_dir / "_ext"))

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
# All restored tutorial notebooks are executed again (#127). The python bindings that
# the remaining deferred notebooks relied on have been finalized in the C++ sources
# (the genuinely missing pieces were added, the notebook bodies migrated to the current
# two-level assembly API):
#
# * example__MNS2002_estimates.md / example__ESV2007_estimates.md: the local a-posteriori
#   indicator assembly (`Operator += LocalElement/IntersectionBilinearFormIndicatorOperator`)
#   has been re-enabled in operators/operator.cc -- it has no BilinearForm-based replacement.
# * example__ESV2007_estimates.md: additionally, the `oswald_interpolation` (interpolations/
#   oswald.cc) and `LaplaceIpdgFluxReconstructionOperator` (operators/
#   laplace_ipdg_flux_reconstruction.cc) bindings have been re-enabled, and the notebook's
#   stiffness assembly migrated to the `BilinearForm` + `append` API.
# * example__ipdg_heat_equation.md: the time-stepping cells form `m_h + dt*a_h`, i.e. a
#   `GDT::(Const)LincombOperator`; those pybind types are now registered in
#   operators/interfaces_all_grids.hh.
# * example__gmsh_grid.md: `meshio` is installed in the CI job and used by the notebook to
#   re-write the mesh produced by pymor's `discretize_gmsh` into a version 2.2 file that
#   dune-grid can read (independent of the runner's gmsh version).
nb_execution_excludepatterns = []

# -----------------------------------------------------------------------------
# clangquill C++ API documentation
# -----------------------------------------------------------------------------
# Parse the in-tree dune-gdt / dune-xt C++ headers with libclang and render MyST
# pages that Sphinx indexes through its C++ domain. This replaces the former
# Doxygen-based C++ API setup (the removed doc/doxygen target). The generated
# pages are written under this srcdir into clangquill_output_dir and pulled into
# the manual through the cpp_api/index toctree entry in index.md.
#
# All clangquill paths are resolved relative to this srcdir (docs/source),
# so "../.." is the repository root: the input glob covers dune/{gdt,xt}/**/*.hh
# and the include dir lets the intra-tree `#include <dune/...>` headers resolve.
# The compilation database below does hold entries for some in-tree headers:
# dune-common's ENABLE_HEADERCHECK mechanism compiles one generated
# <build>/headercheck/<...>.hh.cc stub per header, keyed to the header it
# checks -- confirmed directly from the diagnostics log, which reports flags
# belonging to headercheck__dune_gdt_algorithms_newton.hh. Third-party headers
# (boost, Eigen, gtest, tbb, config.h, pybind11) have no such stub -- nothing
# in this build compiles them standalone -- so they always fall through to the
# clangquill_std / clangquill_include_dirs / clangquill_clang_resource_dir
# fallback below.
# A matched header's database entry was produced by whatever compiler the
# configure below used; if that differs from the libclang clangquill parses
# with, its driver flags can include ones libclang rejects outright as
# "unknown argument" -- this is why the docs job configures with a clang
# preset (see non_docker_build.yml) rather than the gcc-based default.
# libclang reports every include it cannot resolve, and every flag it
# rejects, as errors, while still extracting every symbol it can parse around
# them. Those diagnostics are deliberately
# *not* suppressed -- they are real gaps in what the C++ API pages can document,
# so the -W build surfaces them rather than hiding them. Should the wheel be
# built without libclang, the extension writes a placeholder page and warns
# (clangquill.libclang); that warning is deliberately left unfiltered, so a
# build that would silently publish the manual without any C++ API at all fails
# under -W instead of quietly shipping the placeholder.

# The C++ standard and include dirs the in-tree headers are parsed with.
# clangquill falls back to these -- plus clangquill_clang_resource_dir, which
# only ever reaches libclang on this same fallback path -- for every input the
# compilation database holds no entry for: every third-party header (boost,
# Eigen, gtest, tbb, config.h, pybind11 -- nothing compiles them standalone,
# so no database entry ever names them) and any in-tree header dune-common's
# ENABLE_HEADERCHECK stubs do not cover. clangquill's only near-miss lookup
# beyond an exact path match is a same-directory <stem>.cpp -- of which this
# repository has none, so it never fires. A database is required all the
# same: since clangquill 0.10 the Sphinx extension refuses to guess compile
# flags and aborts without one.
# Tracks CMAKE_CXX_STANDARD in CMakePresets.json: the headers are written
# against that standard, so parsing them at an older one fails on constructs
# that are perfectly valid in the build.
_cpp_std = "c++20"
_cpp_include_dirs = ["../.."]


def _clang_resource_dir():
    """Locate the clang builtin-header directory (``stddef.h`` & co.).

    clangquill's bundled libclang ships no builtin headers, so any system
    ``#include`` (``<cstddef>``, ``<vector>``, ...) fails with ``'stddef.h' file
    not found`` unless we point clang at a resource directory. Prefer an explicit
    ``CLANGQUILL_CLANG_RESOURCE_DIR`` override, otherwise ask ``clang`` on
    ``PATH`` where its builtins live, preferring ``clang-22`` -- the version
    clangquill currently bundles (libclang-*.so.22.1.8) -- since a resource dir
    from a different clang major version can be missing headers this libclang
    expects, or carry ones it does not. Returns ``None`` when none is found,
    which leaves clang to its own (here unset) default — generation still
    runs, just with more diagnostics.
    """
    import shutil  # noqa: PLC0415
    import subprocess  # noqa: PLC0415

    override = os.environ.get("CLANGQUILL_CLANG_RESOURCE_DIR")
    if override:
        return override
    clang = (
        shutil.which("clang-22")
        or shutil.which("clang")
        or shutil.which("clang-18")
        or shutil.which("clang-19")
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


def _compile_commands_dir():
    """Locate the CMake-exported compilation database clangquill parses with.

    Every preset configures with ``CMAKE_EXPORT_COMPILE_COMMANDS=ON`` into
    ``build/<preset>/`` (see CMakePresets.json), and CMake's Ninja generator
    writes the database at *generate* time -- configuring a build tree is
    enough, nothing has to be compiled. ``CLANGQUILL_COMPILE_COMMANDS`` names
    one explicitly (a directory holding a ``compile_commands.json``, or that
    file itself); otherwise the most recently configured build tree is used. A
    database that exists but is unreadable, malformed or empty is rejected by
    clangquill itself with a message naming the file, so only the
    nothing-found case is handled here.
    """
    override = os.environ.get("CLANGQUILL_COMPILE_COMMANDS")
    if override:
        return override
    build_dir = _repo_root / "build"
    # A CMakeCache.txt beside it is what makes a directory a configured build
    # tree; it also keeps any hand-written database from being picked up.
    databases = [
        path
        for path in build_dir.glob("*/compile_commands.json")
        if (path.parent / "CMakeCache.txt").is_file()
    ]
    if not databases:
        raise ConfigError(
            "no CMake compilation database found. The C++ API pages are parsed "
            "with clangquill, which requires one and does not guess compile "
            "flags.\n"
            f"Looked for: {build_dir}/*/compile_commands.json (next to a "
            "CMakeCache.txt)\n"
            "Configure a build tree once -- configuring is enough, nothing has "
            "to be compiled:\n"
            "    cmake --preset=release\n"
            "(any preset in CMakePresets.json exports the database; see "
            "`cmake --list-presets`). Alternatively point "
            "CLANGQUILL_COMPILE_COMMANDS at an existing database (a directory "
            "holding a compile_commands.json, or that file itself)."
        )
    # Newest wins: the tree configured last is the one being worked in.
    return str(max(databases, key=lambda path: path.stat().st_mtime).parent)


clangquill_input = ["../../dune/**/*.hh"]
clangquill_include_dirs = _cpp_include_dirs
clangquill_std = _cpp_std
clangquill_clang_resource_dir = _clang_resource_dir()
clangquill_compile_commands = _compile_commands_dir()
clangquill_output_dir = "cpp_api"
# Build a browsable namespace hierarchy (clangquill >= 0.6.0): the index lists
# only the top namespaces, each namespace hub links its sub-namespaces, classes,
# per-name function pages, a lumped operators page, and grouped types/constants
# pages. This replaces the single colossal flat index (one entry per class) with
# a drill-down: all namespaces -> everything in a namespace -> per-symbol pages.
clangquill_group_by = "namespace"
# Emit every symbol clangquill extracts, including the (largely templated)
# undocumented internals, so the C++ API pages cover all in-tree headers rather
# than only those carrying a documentation comment.
clangquill_include_undocumented = True
# Persist the SQLite IR + page hashes so local rebuilds are incremental.
clangquill_cache_dir = "_clangquill_cache"
# Full libclang diagnostics of the run -- every severity, plus the `note:` chain
# attached to each -- written here (clangquill >= 0.14). The Sphinx warning
# stream only ever carries the errors, and only their top-level message, so this
# file is where the context needed to actually fix one lives: which include
# failed, and the notes explaining what it made unparseable. Written under the
# git-ignored build/ next to the generated compilation database rather than into
# the srcdir, and uploaded by the docs CI job (see non_docker_build.yml) so a red
# build's diagnostics are retrievable instead of only scrollable in the log.
clangquill_diagnostics_log = "../../build/clangquill-diagnostics.log"

# -----------------------------------------------------------------------------
# Warnings
# -----------------------------------------------------------------------------
# The docs are built with `-W` (see the build_docs job in
# .github/workflows/non_docker_build.yml), so every warning fails the build. Only
# warnings the documentation itself cannot fix are filtered out -- by type here,
# and by where they come from in _UnactionableWarningFilter below. libclang's
# parse diagnostics (clangquill.parse) are deliberately *not* among them: they
# mark C++ the API pages could not read, which is fixable by making the missing
# headers available to the parse, so the build reports them.
suppress_warnings = [
    # Cosmetic, and only reachable when the notebooks are *not* executed. The
    # tutorials use IPython shell escapes (`!ls -l f.vtu`), which the plain
    # "python" pygments lexer cannot tokenise. Executing a notebook records
    # language_info.pygments_lexer = "ipython3" -- which handles them -- so this
    # never fires in CI; without execution the lexer falls back to
    # kernelspec.language ("python") and the escapes fail to lex. Sphinx retries
    # in relaxed mode either way, so the rendered page is fine.
    "misc.highlighting_failure",
]


def _warning_location(record):
    """Best-effort source location of a Sphinx log ``record``, as a string.

    ``location=`` is passed as a docname, a ``(docname, lineno)`` pair or a
    docutils node, depending on who logs the warning; Sphinx normalises all of
    them, but only in a handler filter that runs after the one counting warnings
    (see :func:`setup`), so this filter has to do it itself.
    """
    location = getattr(record, "location", None)
    if location is None:
        return ""
    if isinstance(location, tuple):
        return str(location[0] or "")
    if isinstance(location, str):
        return location
    return sphinx.util.logging.get_node_location(location) or ""


def _inventories_are_all_https(mapping):
    """Whether every configured intersphinx inventory is fetched over HTTPS.

    Guards the inventory-failure rule in :class:`_UnactionableWarningFilter`:
    Sphinx reports *any* exhausted inventory the same way, so with a local
    ``objects.inv`` configured that one warning would also cover an unreadable or
    malformed file -- a real misconfiguration, and one this build should fail on.
    While every target is a URL, the warning can only mean the network. A
    plain-``http://`` target deliberately does not qualify: this project has none,
    and the stricter reading (inventory failures stay fatal) is the safe default.
    """
    for target_uri, inventories in mapping.values():
        if not isinstance(inventories, tuple):
            inventories = (inventories,)
        for location in (target_uri, *inventories):
            if location is not None and not location.strip().startswith("https://"):
                return False
    return True


class _UnactionableWarningFilter(logging.Filter):
    """Drop the warnings this documentation build has no way to act on.

    Everything else stays fatal under `-W`; the two exceptions are:

    * Warnings about a page under ``clangquill_output_dir``. Those pages are
      generated from the C++ headers, and the bulk of them are the Sphinx C++
      domain balking at declarations libclang extracted verbatim (deeply nested
      DUNE templates it cannot re-parse, specialisations it sees as duplicates)
      or MyST reading braces in C++ code as a substitution. None of that is
      fixable from this repository, and none of it comes from a hand-written
      page -- which keeps failing the build for its own warnings.
    * intersphinx exhausting the locations for an inventory, which with the
      all-HTTPS mapping this project configures means the network was in the
      way rather than the documentation. That warning carries no type/subtype,
      so ``suppress_warnings`` cannot address it; every intersphinx warning that
      does report a documentation problem (an unresolvable cross-reference) is
      typed, so dropping only the untyped ones keeps `-W` meaningful here too.
      Should a local inventory ever be configured, the same warning starts
      covering an unreadable or malformed file as well, and this rule turns
      itself off rather than hide it.
    """

    def __init__(self, generated_dir, *, filter_inventory_failures):
        super().__init__()
        self._generated = f"{generated_dir}/"
        self._filter_inventory_failures = filter_inventory_failures

    def filter(self, record):
        if record.levelno < logging.WARNING:
            return True
        if (
            self._filter_inventory_failures
            and "intersphinx" in record.name
            and getattr(record, "type", None) is None
        ):
            return False
        location = _warning_location(record).replace(os.sep, "/")
        return not (
            location.startswith(self._generated) or f"/{self._generated}" in location
        )


def setup(_app):
    """Sphinx entry point for conf.py-local setup (called with the app)."""
    # Sphinx counts a warning -- and, under `-W`, fails the build for it -- in a
    # filter on the handlers of its "sphinx" logger, so ours has to run before
    # those: insert it at the front of each chain rather than appending it.
    # intersphinx_mapping is defined further down; setup() runs after conf.py has
    # been executed in full, so the module global is there by then.
    warning_filter = _UnactionableWarningFilter(
        clangquill_output_dir,
        filter_inventory_failures=_inventories_are_all_https(intersphinx_mapping),
    )
    for handler in logging.getLogger("sphinx").handlers:
        handler.filters.insert(0, warning_filter)


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
    "Sphinx": ("https://www.sphinx-doc.org/en/master/", None),
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


def linkcode_resolve(domain, info):
    if domain != "py" or not info["module"]:
        return None
    parts = info["module"].split(".")
    # dune.gdt / dune.xt bindings live under python/<gdt|xt>/dune/<gdt|xt>/...,
    # while the documentation helper modules sit next to this conf.py in
    # docs/source/.
    if parts[:2] == ["dune", "gdt"]:
        rel = Path("python", "gdt", *parts)
    elif parts[:2] == ["dune", "xt"]:
        rel = Path("python", "xt", *parts)
    else:
        rel = Path("docs", "source", *parts)
    # a package resolves to its __init__.py, a plain module to <name>.py
    if (_repo_root / rel / "__init__.py").is_file():
        rel = rel / "__init__.py"
    else:
        rel = rel.with_suffix(".py")
    return f"{_linkcode_baseurl}/tree/{branch}/{rel.as_posix()}"

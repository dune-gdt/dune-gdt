# -W turns warnings into errors --keep-going lets sphinx process the full build instead of aborting on first warning
# this is esp. necessary for myst-nb code-cells that otherwise show no visible code error
set(SPHINXOPTS -W --keep-going -v -j auto)

set(SPHINX_BUILDDIR ${CMAKE_CURRENT_BINARY_DIR}/html)
set(SPHINX_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/source)
add_custom_target(docs COMMAND ${RUN_IN_ENV_SCRIPT} sphinx-build ${SPHINX_SOURCE_DIR} -b html ${SPHINXOPTS}
                               ${SPHINX_BUILDDIR})
add_dependencies(docs bindings)

# ----------------------------------------------------------------------------------------------------
# notebook scripts: the documentation notebooks as individually runnable Python scripts
# ----------------------------------------------------------------------------------------------------
# Executing the notebooks is part of the `docs` target above and doubles as an integration test of the Python bindings,
# but it is a bad test to debug: myst-nb runs every notebook through a Jupyter kernel inside one long sphinx-build, and
# ipykernel replaces the kernel process' fd 1/2 with pipes drained by an in-process thread. A kernel that dies from a
# deadly signal therefore loses whatever faulthandler wrote into that pipe, and the only thing surfacing to the docs
# build is `nbclient.exceptions.DeadKernelError: Kernel died` -- no traceback, no cell, no way to re-run just the one
# notebook.
#
# notebooks_to_scripts.py transcribes the code cells of each notebook into a standalone script that runs in a plain
# interpreter (fd 2 goes straight to the test log, so faulthandler dumps survive) and announces every cell before
# running it. Each script is registered as its own CTest test, so `ctest -L notebook` reproduces the docs-build notebook
# execution with per-notebook isolation, timeouts and exit statuses.
#
# Build the `notebook_scripts` target before running those tests: it regenerates the scripts from the notebook sources
# and (like `docs`) builds the bindings they import. CTest never builds dependencies itself.
set(NOTEBOOK_CONVERTER ${CMAKE_CURRENT_SOURCE_DIR}/notebooks_to_scripts.py)
set(NOTEBOOK_SCRIPT_DIR ${CMAKE_CURRENT_BINARY_DIR}/notebook_scripts)

# The per-notebook tests have to be registered at configure time, so ask the converter up front which notebooks there
# are (--list) and which files they live in (--list-paths, the dependencies of the generation step below).
execute_process(
  COMMAND ${Python_EXECUTABLE} ${NOTEBOOK_CONVERTER} --source-dir ${SPHINX_SOURCE_DIR} --list
  OUTPUT_VARIABLE _notebook_names
  ERROR_VARIABLE _notebook_error
  RESULT_VARIABLE _notebook_result
  OUTPUT_STRIP_TRAILING_WHITESPACE)
if(NOT _notebook_result EQUAL 0)
  message(FATAL_ERROR "listing the documentation notebooks failed: ${_notebook_error}")
endif()
execute_process(
  COMMAND ${Python_EXECUTABLE} ${NOTEBOOK_CONVERTER} --source-dir ${SPHINX_SOURCE_DIR} --list-paths
  OUTPUT_VARIABLE _notebook_paths
  RESULT_VARIABLE _notebook_paths_result
  OUTPUT_STRIP_TRAILING_WHITESPACE)
if(NOT _notebook_paths_result EQUAL 0)
  message(FATAL_ERROR "listing the documentation notebook paths failed")
endif()
string(REPLACE "\n" ";" NOTEBOOK_NAMES "${_notebook_names}")
string(REPLACE "\n" ";" NOTEBOOK_SOURCES "${_notebook_paths}")

# Re-run the configure step when a notebook is added, removed or edited, so the set of registered tests stays in sync
# with docs/source without a manual `cmake .`.
set_property(
  DIRECTORY
  APPEND
  PROPERTY CMAKE_CONFIGURE_DEPENDS ${NOTEBOOK_CONVERTER} ${NOTEBOOK_SOURCES})

set(NOTEBOOK_SCRIPT_STAMP ${CMAKE_CURRENT_BINARY_DIR}/notebook_scripts.stamp)
add_custom_command(
  OUTPUT ${NOTEBOOK_SCRIPT_STAMP}
  COMMAND ${Python_EXECUTABLE} ${NOTEBOOK_CONVERTER} --source-dir ${SPHINX_SOURCE_DIR} --output-dir
          ${NOTEBOOK_SCRIPT_DIR}
  COMMAND ${CMAKE_COMMAND} -E touch ${NOTEBOOK_SCRIPT_STAMP}
  DEPENDS ${NOTEBOOK_CONVERTER} ${NOTEBOOK_SOURCES} ${SPHINX_SOURCE_DIR}/myst_code_init.py
  COMMENT "Converting the documentation notebooks into standalone scripts"
  VERBATIM)
add_custom_target(notebook_scripts DEPENDS ${NOTEBOOK_SCRIPT_STAMP})
if(TARGET bindings)
  add_dependencies(notebook_scripts bindings)
endif()

# The notebooks run through `uv run` in project mode against the freshly built bindings, exactly like the pytest suites
# in cmake/modules/DuneXTTesting.cmake: WORKING_DIRECTORY is the dune.gdt assembly point in the binary tree (a complete
# editable project from uv's point of view), --python pins the very interpreter the bindings were compiled against, and
# --frozen resolves the notebooks' own dependencies (pyMOR, meshio, k3d, ...) from the committed lockfile through the
# PEP 735 `notebooks` dependency group in python/gdt/pyproject.toml.
#
# The notebooks write files (f.vtu, L_shaped_domain.msh, ...) relative to the working directory, which for uv has to
# stay the assembly point; DUNE_GDT_NOTEBOOK_WORKDIR tells the script to chdir into a scratch directory of its own
# instead, so concurrent tests cannot overwrite each other's output.
set(NOTEBOOK_RUN_DIR ${CMAKE_CURRENT_BINARY_DIR}/notebook_runs)
# DXT_TEST_TIMEOUT is set inside the per-subdirectory test macros in cmake/modules/DuneXTTesting.cmake and so does not
# reach this directory; the notebooks are also a different kind of workload (the interpolation tutorial alone runs for
# ~2.5 min in CI), hence a knob of their own.
set(DXT_NOTEBOOK_TEST_TIMEOUT
    1200
    CACHE STRING "per-notebook timeout (seconds) for the notebook_* CTest tests")
foreach(notebook ${NOTEBOOK_NAMES})
  add_test(NAME notebook_${notebook} COMMAND ${UV_EXECUTABLE} run --frozen --python ${Python_EXECUTABLE} --group
                                             notebooks python ${NOTEBOOK_SCRIPT_DIR}/${notebook}.py)
  set_tests_properties(
    notebook_${notebook}
    PROPERTIES WORKING_DIRECTORY
               ${CMAKE_BINARY_DIR}/python/gdt
               ENVIRONMENT
               "DUNE_GDT_NOTEBOOK_WORKDIR=${NOTEBOOK_RUN_DIR}/${notebook}"
               LABELS
               "dune-gdt-test;notebook"
               TIMEOUT
               ${DXT_NOTEBOOK_TEST_TIMEOUT})
endforeach()

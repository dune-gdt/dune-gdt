# -W turns warnings into errors --keep-going lets sphinx process the full build instead of aborting on first warning
# this is esp. necessary for myst-nb code-cells that otherwise show no visible code error
set(SPHINXOPTS -W --keep-going -v -j auto)

set(SPHINX_BUILDDIR ${CMAKE_CURRENT_BINARY_DIR}/html)
set(SPHINX_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/source)
add_custom_target(tutorials COMMAND ${RUN_IN_ENV_SCRIPT} sphinx-build ${SPHINX_SOURCE_DIR} -b html ${SPHINXOPTS}
                                    ${SPHINX_BUILDDIR})

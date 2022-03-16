ARG BASE
FROM zivgitlab.wwu.io/ag-ohlberger/dune-community/docker/testing-base_${BASE}:bfea00695a81595e3155d7d5702d35b6d9eb0bac

ENV CC=gcc \
    CXX=g++

USER root
ADD . ${SUPERDIR}/
COPY .ci/shared/docker/super_docker/build_bindings.bash /tmp
WORKDIR ${SUPERDIR}
RUN chown dune-ci -R ${SUPERDIR}
USER dune-ci

# run our local scripts look for an OPTS env entry
RUN cd ${SUPERDIR} ; git submodule update --init --recursive  && ls -l bin/ && \
    ./bin/download_external_libraries.py 1> /dev/null && \
    ./bin/build_external_libraries.py 1> /dev/null && \
    . environments/${DXT_ENVIRONMENT}/PATH.sh && \
    ${SRC_DCTRL} ${BLD} all

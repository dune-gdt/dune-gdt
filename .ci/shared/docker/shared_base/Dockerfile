ARG BASE
FROM zivgitlab.wwu.io/ag-ohlberger/dune-community/docker/testing-base_${BASE}

ARG COMMIT
ARG SUPERURL
SHELL ["/bin/bash", "-c"]

RUN git clone ${SUPERURL} ${SUPERDIR} && \
    cd ${SUPERDIR} && \
    git checkout ${COMMIT} && \
    git submodule foreach git reset --hard && \
    git submodule update --init --recursive && \
    git submodule status
WORKDIR ${SUPERDIR}

ARG CC
ARG CXX

ENV CC=${CC} \
    CXX=${CXX}

# run our local scripts look for an OPTS env entry
RUN ./bin/download_external_libraries.py 1> /dev/null && \
    ./bin/build_external_libraries.py 1> /dev/null

RUN . environments/${DXT_ENVIRONMENT}/PATH.sh

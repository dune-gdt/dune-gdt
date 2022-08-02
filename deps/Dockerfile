ARG DEBIAN_BASE=debian
FROM zivgitlab.wwu.io/ag-ohlberger/dune-community/docker/$DEBIAN_BASE-full:517eb0fc771b81afc4484d17442d066d96531f03


USER root
RUN apt update \
    && apt install -y --no-install-recommends python3-venv \
            python3-pip python3-requests openssh-client \
            python3-setuptools wget unzip curl lcov ccache llvm-${CLANG_VERSION} \
            rsync doxygen texlive-latex-base doxygen-latex libmetis-dev \
    && rm -rf /var/lib/apt/lists \
    && python3 -m venv /venv \
    && . /venv/bin/activate \
    && pip install --no-cache wheel pre-commit mpi4py jinja2 numpy scipy

ARG BUILD_OPTS=clang-debug
ARG BUILD_CXX=g++
ARG BUILD_CC=gcc

ENV CONFIG_OPTS=${BUILD_OPTS} \
    CXX=${BUILD_CXX} \
    CC=${BUILD_CC}

COPY . /deps/
RUN . /venv/bin/activate \
    && cd /deps/ \
    && ./dune-common/bin/dunecontrol --opts=config.opts/${CONFIG_OPTS} all \
    && mv entrypoint.bash /usr/local/bin/entrypoint.bash \
    && cd / \
    && rm -rf /root/* \
    && ls -l /venv/bin/ \
    && ls -l /build/ \
    && ls -l /usr/local/bin

WORKDIR /home/dune-ci/
ENTRYPOINT [ "/usr/local/bin/entrypoint.bash" ]

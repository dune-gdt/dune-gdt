ARG DEBIAN_BASE=debian
FROM zivgitlab.wwu.io/ag-ohlberger/dune-community/docker/$DEBIAN_BASE-full:207f7f1627363ac97e11cc209ad1310f3e30b414


USER root
RUN apt update \
    && apt install -y --no-install-recommends python3-venv \
            openssh-client \
            wget unzip curl lcov ccache llvm-${CLANG_VERSION} \
            rsync doxygen texlive-latex-base doxygen-latex libmetis-dev \
    && rm -rf /var/lib/apt/lists \
    && python3 -m venv /venv \
    && . /venv/bin/activate \
    && pip install --no-cache wheel pre-commit mpi4py jinja2 numpy scipy \
    && cd /usr/local/bin  \
    && (for ii in cc cpp clang clang++ clang-${CLANG_VERSION} clang++-${CLANG_VERSION} \
        cpp-${GCC_VERSION} g++ g++-${GCC_VERSION} gcc ; do ln -s $(which ccache) $ii; done)


ARG BUILD_OPTS=clang-debug
ARG BUILD_CXX=g++
ARG BUILD_CC=gcc

ENV CONFIG_OPTS=${BUILD_OPTS} \
    CXX=${BUILD_CXX} \
    CC=${BUILD_CC}

COPY . /src/
RUN . /venv/bin/activate \
    && cd /src/ \
    && ./deps/dune-common/bin/dunecontrol --opts=./deps/config.opts/${CONFIG_OPTS} all \
    && mv deps/entrypoint.bash /usr/local/bin/entrypoint.bash \
    && cd / \
    && rm -rf /root/*

WORKDIR /home/dune-ci/
ENTRYPOINT [ "/usr/local/bin/entrypoint.bash" ]

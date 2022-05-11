ARG DEBIAN_BASE=debian
FROM zivgitlab.wwu.io/ag-ohlberger/dune-community/docker/$DEBIAN_BASE-full:latest

ARG CONFIG_OPTS=clang-debug

USER root
RUN apt update \
    && apt install -y --no-install-recommends python3-venv \
            python3-pip python3-requests openssh-client \
            python3-setuptools wget unzip curl lcov ccache llvm-${CLANG_VERSION} \
            rsync doxygen texlive-latex-base doxygen-latex \
    && rm -rf /var/lib/apt/lists \
    && python3 -m venv /venv

COPY . /src/
RUN . /venv/bin/activate \
    && pip install --no-cache pre-commit mpi4py \
    && cd /src/ \
    && ./dune-common/bin/dunecontrol --opts=config.opts/${CONFIG_OPTS} all \
    && ./dune-common/bin/dunecontrol --opts=config.opts/${CONFIG_OPTS} make install \
    && mv entrypoint.bash /usr/local/bin/entrypoint.bash \
    && cd / \
    && rm -rf /src /root/*

WORKDIR /home/dune-ci/
ENTRYPOINT [ "/usr/local/bin/entrypoint.bash" ]

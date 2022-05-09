ARG DEBIAN_BASE=debian
FROM zivgitlab.wwu.io/ag-ohlberger/dune-community/docker/testing-base_$DEBIAN_BASE:latest

ARG CONFIG_OPTS=clang-debug

USER root
RUN apt update \
    && apt install -y --no-install-recommends python3-venv \
    && rm -rf /var/lib/apt/lists

COPY . /src/
RUN python3 -m venv /venv \
    && . /venv/bin/activate \
    && cd /src/ \
    && ./dune-common/bin/dunecontrol --opts=config.opts/${CONFIG_OPTS} all \
    && ./dune-common/bin/dunecontrol --opts=config.opts/${CONFIG_OPTS} make install \
    && mv entrypoint.bash /usr/local/bin/entrypoint.bash \
    && cd / \
    && rm -rf /src /root/*
USER dune-ci

WORKDIR /src
ENTRYPOINT [ "/usr/local/bin/entrypoint.bash" ]

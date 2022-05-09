ARG DEBIAN_BASE=debian-full
FROM zivgitlab.wwu.io/ag-ohlberger/dune-community/docker/$DEBIAN_BASE:latest

ARG OPTS=clang-debug

RUN apt update \
    && apt install -y --no-install-recommends python3-venv \
    && rm -rf /var/lib/apt/lists

COPY . /src/
RUN python3 -m venv /venv \
    && . /venv/bin/activate \
    && cd /src/ \
    && ./dune-common/bin/dunecontrol --opts=config.opts/${OPTS} all \
    && ./dune-common/bin/dunecontrol --opts=config.opts/${OPTS} make install \
    && mv entrypoint.bash /usr/local/bin/entrypoint.bash \
    && cd / \
    && rm -rf /src /root/*

WORKDIR /src
ENTRYPOINT [ "/usr/local/bin/entrypoint.bash" ]

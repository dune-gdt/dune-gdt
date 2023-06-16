# The hash below gets set when calling ./docker/build_dev.sh
FROM ghcr.io/dune-gdt/dune-gdt/dev:8aedb5a386ad2428c6e730623ce3aa5e976496854c4ff344ff2aaa82bed3ba49

# only update the DUNE_GDT_REF if any of SUBMODULES_HASH or OPTS_HASH changes (see docker/build_ci.sh)!
ENV \
    DEBIAN_FRONTEND=noninteractive \
    DUNE_GDT_REF=0ca387e690d46fa7fc46a4ec364363726cb30134
SHELL ["/bin/bash", "-ec"]

USER root

RUN \
    git clone https://github.com/dune-gdt/dune-merged.git /source && \
    cd /source && git checkout ${DUNE_GDT_REF} && \
    git submodule update --init --recursive && \
    for OPTS in gcc-debug.ninja; do \
        virtualenv -p python3.9 /venvs/venv-${OPTS} && \
        echo 'export DCTL_ARGS="--opts=./deps/config.opts/${OPTS} --builddir=/build/${OPTS}"' >> /venvs/venv-${OPTS}/bin/activate && \
        echo "cd /source" >> /venvs/venv-${OPTS}/bin/activate \
        . /venvs/venv-${OPTS}/bin/activate && \
        WERROR=0 ./deps/dune-common/bin/dunecontrol ${DCTL_ARGS} configure && \
        ./deps/dune-common/bin/dunecontrol ${DCTL_ARGS} all \
    ; done && \
    rm -rf /build/${OPTS}/dune-gdt

RUN \
    mv /data/bashrc /root/.bashrc && \
    mv /data/bash-ps1 /root/.bash-ps1 && \
    chown root:root /root/.bashrc /root/.bash-ps1 && \
    chmod 644 /root/.bashrc /root/.bash-ps1 && \
    mkdir -p /root/.ccache && \
    echo "max_size = 5.0G" > /root/.ccache/ccache.conf && \
    rm -rf /data

ADD entrypoint.sh /usr/local/bin/
ENTRYPOINT ["/usr/local/bin/entrypoint.sh"]

# see http://label-schema.org/rc1/#build-time-labels
LABEL org.label-schema.build-date=2023-06-14T09:14:39Z \
      org.label-schema.name=debian-full \
      org.label-schema.maintainer=felix.schindler@uni-muenster.de \
      org.label-schema.description="CI environment for dune-gdt" \
      org.label-schema.url="https://github.com/dune-gdt/dune-gdt" \
      org.label-schema.vcs-ref=b1b052c345d0d9866c1d30d8b20264018861612a \
      org.label-schema.vcs-url="https://github.com/dune-gdt/dune-gdt" \
      org.label-schema.schema-version="1.0" \
      org.label-schema.usage="https://github.com/dune-gdt/dune-gdt/blob/master/README.md"

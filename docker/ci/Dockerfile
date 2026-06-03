FROM ghcr.io/dune-gdt/dune-gdt/dev:FROM_DEV_HASH

# only update the DUNE_GDT_REF if any of the hashes in docker/compute_ci_hash.sh apart from CI_HASH change!
ENV \
    DEBIAN_FRONTEND=noninteractive \
    DUNE_GDT_REF=0ca387e690d46fa7fc46a4ec364363726cb30134
SHELL ["/bin/bash", "-ec"]

USER root

RUN \
    apt-get update -qq 1> /dev/null && \
    apt-get install -q=100 -y --no-install-recommends \
        xauth \
        xvfb \
        && \
    apt-get autoremove -y 1> /dev/null && \
    apt-get clean && \
    rm -rf /var/lib/apt/lists/*

# We prepare re-usable venvs that can be moved into the build directory and adapted
RUN \
    git clone https://github.com/dune-gdt/dune-gdt.git /source && \
    cd /source && git checkout ${DUNE_GDT_REF} && \
    git submodule update --init --recursive && \
    rm -rf cmake \
        CMakeLists.txt \
        config.h.cmake \
        CONTRIBUTING.md \
        doc \
        docker \
        docker-compose.yml \
        dune \
        dune-gdt.pc.in \
        dune.module \
        examples \
        LICENSE \
        make_docker_wheels.bash \
        merge.bash \
        python \
        README.md \
        stamp-vc \
        tutorials \
        && \
    for OPTS in gcc-debug.ninja; do \
        export VENVDIR="/build/${OPTS}/venv" && \
        virtualenv -p python3.9 "${VENVDIR}" && \
        echo "export DUNECONTROL=\"/source/deps/dune-common/bin/dunecontrol --opts=./deps/config.opts/${OPTS} --builddir=/build/${OPTS} --only=dune-gdt \"" > "${VENVDIR}.sh" && \
        echo ". /venvs/venv-${OPTS}/bin/activate" >> "${VENVDIR}.sh" && \
        echo "cd /source" >> "${VENVDIR}.sh" && \
        . "${VENVDIR}.sh" && \
        python -m pip install mpi4py==4.0.3 -i https://pypi.anaconda.org/mpi4py/simple && \
        WERROR=0 /source/deps/dune-common/bin/dunecontrol --opts=./deps/config.opts/${OPTS} --builddir=/build/${OPTS} configure && \
        /source/deps/dune-common/bin/dunecontrol --opts=./deps/config.opts/${OPTS} --builddir=/build/${OPTS} all && \
        mkdir /venvs/${OPTS} && \
        mv "${VENVDIR}" "/venvs/${OPTS}/" \
    ; done

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
LABEL org.label-schema.build-date=LABEL_BUILD_DATE \
      org.label-schema.name=ci \
      org.label-schema.maintainer=felix.schindler@uni-muenster.de \
      org.label-schema.description="CI environment for dune-gdt" \
      org.label-schema.url="https://github.com/dune-gdt/dune-gdt" \
      org.label-schema.vcs-ref=LABEL_VCS_REF \
      org.label-schema.vcs-url="https://github.com/dune-gdt/dune-gdt" \
      org.label-schema.schema-version="1.0" \
      org.label-schema.usage="https://github.com/dune-gdt/dune-gdt/blob/master/README.md" \
      org.opencontainers.image.source="https://github.com/dune-gdt/dune-gdt"

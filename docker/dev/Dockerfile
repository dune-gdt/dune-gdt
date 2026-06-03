FROM debian:bullseye-20221004-slim

# install locales before anything else to supress warnings
ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update -qq 1> /dev/null && \
    apt-get upgrade -qqy --no-install-recommends 1> /dev/null && \
    echo "en_US.UTF-8 UTF-8" >> /etc/locale.gen && \
    echo "de_DE.UTF-8 UTF-8" >> /etc/locale.gen && \
    apt-get install -q=100 -y --no-install-recommends locales 1> /dev/null && \
    dpkg-reconfigure locales

# OMPI_ALLOW_RUN_AS_ROOT* is required for root in rootless docker

# then do the rest
ENV DXT_ENVIRONMENT=debian-full \
    OMPI_ALLOW_RUN_AS_ROOT=1 \
    OMPI_ALLOW_RUN_AS_ROOT_CONFIRM=1 \
    CLANG_VERSION=15 \
    GCC_VERSION=10 \
    CLANG_PYTHON_PACKAGE=python3-clang \
    DXT_DOCKER=1 \
    LANGUAGE=en_US.UTF-8 \
    LANG=en_US.UTF-8 \
    LC_ALL=en_US.UTF-8 \
    LC_CTYPE=en_US.UTF-8 \
    DXT_LIBCLANG_SO=/usr/lib/llvm-${CLANG_VERSION}/lib/libclang.so.1 \
    CCACHE_COMPILERCHECK=content \
    CCACHE_COMPRESS=true \
    MOLD_VERSION=1.1.1 \
    CLANG_TOOLING_VERSION=8

RUN echo "deb http://deb.debian.org/debian stable contrib non-free" > /etc/apt/sources.list.d/stable-nonfree.list && \
    apt-get update -qq 1> /dev/null && \
    apt-get install -q=100 -y --no-install-recommends \
        automake \
        bison \
        build-essential \
        ca-certificates \
        cmake \
        cmake \
        cmake-curses-gui \
        flex \
        g++-${GCC_VERSION} \
        gcc-${GCC_VERSION} \
        gfortran \
        gfortran-${GCC_VERSION} \
        git \
        gosu \
        libboost-chrono-dev \
        libboost-date-time-dev \
        libboost-filesystem-dev \
        libboost-system-dev \
        libboost-thread-dev \
        libboost-timer-dev \
        libglu1-mesa\
        libgraphviz-dev \
        libopenblas-dev \
        libopengl-dev \
        libsuperlu-dev \
        libtool \
        ninja-build \
        pkg-config \
        python3 \
        python3-dbg \
        python3-dev \
        python3-pip \
        python3-tk \
        python3-venv \
        python3-virtualenv \
        sudo \
        unzip \
        virtualenv \
        wget \
        1> /dev/null && \
    update-alternatives --install /usr/bin/python python /usr/bin/python3 100
RUN cd /tmp && git clone https://github.com/dune-community/cli-setup && \
    cd cli-setup && \
    cmake . && make install && rm -rf /tmp/cli-setup
RUN wget https://github.com/rui314/mold/releases/download/v${MOLD_VERSION}/mold-${MOLD_VERSION}-x86_64-linux.tar.gz \
    -O- | tar zx --strip-components=1 -C /usr/local
RUN git clone --depth=1 https://github.com/wjakob/tbb /tmp/tbb && \
    mkdir /tmp/tbb_build && cd /tmp/tbb_build && \
    cmake -GNinja /tmp/tbb -DTBB_BUILD_PYTHON=ON -DTBB_NO_DATE=ON -DCMAKE_INSTALL_PREFIX=/usr/local && \
    ninja -j $(nproc --ignore=1) -l $(nproc --ignore=2) && \
    ninja install && \
    rm -rf /tmp/*
# M4_LIBCPP_DEV is defined in include/minimal because older packages were not versioned
# CLANG_FORMAT is defined as a commonly available version
RUN apt-get install -q=100 -y --no-install-recommends doxygen libc++-dev \
        libeigen3-dev libscotchmetis-dev libscotchparmetis-dev libsuitesparse-dev \
        libmumps-scotch-dev lcov curl \
        ninja-build libopenmpi-dev openmpi-common openmpi-bin \
        lsb-release wget software-properties-common gnupg \
        doxygen python3-pip python3-requests doxygen-latex python3-setuptools \
        valgrind-mpi liblapacke-dev ssh numactl libtcmalloc-minimal4 google-perftools  1> /dev/null && \
    wget https://apt.llvm.org/llvm.sh && \
    chmod +x llvm.sh && \
    ./llvm.sh ${CLANG_VERSION}
RUN \
    update-alternatives --install /usr/bin/clang++ clang++ /usr/bin/clang++-${CLANG_VERSION} 100 && \
    update-alternatives --install /usr/bin/clang clang /usr/bin/clang-${CLANG_VERSION} 100 && \
    wget https://codecov.io/bash -O /usr/local/bin/codecov && \
    chmod +x /usr/local/bin/codecov && \
    rm -rf /var/lib/apt/lists/*
RUN \
    wget -O /usr/local/bin/clang-tidy-${CLANG_TOOLING_VERSION} \
      https://github.com/muttleyxd/clang-tools-static-binaries/releases/download/master-22538c65/clang-tidy-${CLANG_TOOLING_VERSION}_linux-amd64 && \
    wget -O /usr/local/bin/clang-format-${CLANG_TOOLING_VERSION} \
      https://github.com/muttleyxd/clang-tools-static-binaries/releases/download/master-22538c65/clang-format-${CLANG_TOOLING_VERSION}_linux-amd64 && \
    chmod +x /usr/local/bin/clang-*
RUN \
    apt-get autoremove -y 1> /dev/null && \
    apt-get clean && \
    rm -rf /var/lib/apt/lists/*

# clean up root home and add files for custom setup
RUN \
    rm -f /root/.wget-hsts /root/.bashrc
COPY bashrc bash-ps1 /data/
ADD openmpi-mca-params.conf /etc/openmpi/
ADD entrypoint.sh /usr/local/bin/
ENTRYPOINT ["/usr/local/bin/entrypoint.sh"]

# see http://label-schema.org/rc1/#build-time-labels
LABEL org.label-schema.build-date=LABEL_BUILD_DATE \
      org.label-schema.name=dev \
      org.label-schema.maintainer=felix.schindler@uni-muenster.de \
      org.label-schema.description="Development environment for dune-gdt" \
      org.label-schema.url="https://github.com/dune-gdt/dune-gdt" \
      org.label-schema.vcs-ref=LABEL_VCS_REF \
      org.label-schema.vcs-url="https://github.com/dune-gdt/dune-gdt" \
      org.label-schema.schema-version="1.0" \
      org.label-schema.usage="https://github.com/dune-gdt/dune-gdt/blob/master/README.md" \
      org.opencontainers.image.source="https://github.com/dune-gdt/dune-gdt"

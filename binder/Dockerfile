FROM dunecommunity/xt-super_debian:master
MAINTAINER rene.fritze@wwu.de

ARG NB_USER
ARG NB_UID
ARG DXT_JUPYTER_TOKEN

USER root

# /home/dune-ci already exists in the base image and needs to be re-used here
RUN useradd -d /home/dune-ci --shell /bin/bash -u ${NB_UID} -o -c "" -m ${NB_USER} && \
    chown -R ${NB_USER} /home/dune-ci

ENV JUPYTER_TOKEN=${DXT_JUPYTER_TOKEN} \
    USER=${NB_USER} \
    HOME=/home/dune-ci \
    VENV=${DUNE_BUILD_DIR}/dune-common/dune-env

USER ${NB_USER}

ENV PATH=${VENV}/bin:${PATH}

ENTRYPOINT []
WORKDIR ${HOME}

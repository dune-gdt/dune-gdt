FROM zivgitlab.wwu.io/ag-ohlberger/dune-community/docker/debian-qtcreator

MAINTAINER Felix Schindler <felix dot schindler at uni-muenster dot de>

ENV DEBIAN_FRONTEND=noninteractive
SHELL ["/bin/bash", "-ec"]

USER root
# add files required for custom user setup
COPY bashrc bash-ps1 /data/
ADD entrypoint.sh /usr/local/bin/

ENV DEBIAN_FRONTEND teletype
ENTRYPOINT ["/usr/local/bin/entrypoint.sh"]


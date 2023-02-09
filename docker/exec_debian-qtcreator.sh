#!/bin/bash
#

CONTAINER=dune-gdt-debian-qtcreator
CID_FILE=/tmp/${CONTAINER}_${USER}.cid
DOCKER_BIN=${DOCKER:-docker}

${DOCKER_BIN} exec -it $(cat ${CID_FILE}) /bin/bash


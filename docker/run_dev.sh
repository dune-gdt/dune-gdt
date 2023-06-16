#!/bin/bash

# the dir containing this script is needed for mounting stuff into the container
PROJECTDIR="$(cd "$(dirname ${BASH_SOURCE[0]})/.." ;  pwd -P)"
CONTAINER=ghcr.io/dune-gdt/dune-gdt/dev
CID_FILE=/tmp/${CONTAINER//\//_}_${USER}.cid
PORT="18$(( ( RANDOM % 10 ) ))$(( ( RANDOM % 10 ) ))$(( ( RANDOM % 10 ) ))"
DOCKER_BIN=${DOCKER:-docker}

SOURCE=${PROJECTDIR}
TARGET="/root/dune-gdt"
if [ $# -eq 0 ] ; then
  EXEC="/bin/bash"
else
  EXEC="${@}"
fi
DOCKER_HOME="${HOME}/.docker-homes/${CONTAINER//\//_}"


if [ -e ${CID_FILE} ]; then

  echo "An dev docker container for dune-gdt is already running."
  echo "Execute"
  echo "  ./${PROJECTDIR}/exec_dev.sh"
  echo "to connect to it."
  echo
  echo "If you are absolutely certain that there is no running container"
  echo "(check with '${DOCKER_BIN} ps' and stop it otherwise), you may"
  echo "  rm $CID_FILE"
  echo "and try again."

else

  echo "======================================================================================"
  echo "  Starting a dev docker container for dune-gdt"
  echo "  - based on container ${CONTAINER}"
  echo "  - with exposed port ${PORT}"
  echo "  The root user within the container is mapped to the current user $USER on the host"
  echo "  - files created as root within the container will belong to $USER on the host"
  echo "  - commands executed as root within the container will be executed with permissions"
  echo "    of user $USER on the host."
  echo "  Mounting"
  echo "    ${DOCKER_HOME}"
  echo "      -> /root"
  echo "    ${SOURCE}"
  echo "      -> ${TARGET}  (all commands are executed within this directory)"
  echo "  so that all changes within these folders are persistent (generally applies to most"
  echo "  config files written to dotfiles)!"
  echo "  To connect to this container, execute"
  echo -n "      ${DOCKER_BIN} exec -it "
  echo -n                               '$(cat '
  echo -n                                      "${CID_FILE}"
  echo                                                     ') /bin/bash'
  echo "======================================================================================"
  echo

  mkdir -p ${DOCKER_HOME} &> /dev/null

  ${DOCKER_BIN} run --rm=true --privileged=true -t -i --hostname docker --cidfile=${CID_FILE} \
    -e LOCAL_USER=root -e LOCAL_UID=0 -e LOCAL_GID=0 \
    -e DISPLAY=$DISPLAY -v /tmp/.X11-unix:/tmp/.X11-unix \
    -e QT_X11_NO_MITSHM=1 \
    -e QT_SCALE_FACTOR=${QT_SCALE_FACTOR:-1} \
    -e GDK_DPI_SCALE=${GDK_DPI_SCALE:-1} \
    -e EXPOSED_PORT=$PORT -p 127.0.0.1:$PORT:$PORT \
    -v /etc/localtime:/etc/localtime:ro \
    -v "${DOCKER_HOME}":/root \
    -v "${SOURCE}":"${TARGET}" \
    ${CONTAINER} "cd ~/dune-gdt && ${EXEC}"

  echo
  echo "Removing ${CID_FILE}"
  rm -f ${CID_FILE}
  exit 0

fi


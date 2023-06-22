#!/bin/bash

if [[ "X${OPTS}" == "X" ]] ; then
    >&2 echo "ERROR: need to specify one of deps/config.opts/ as OPTS"
    exit 1
fi

export LANG=en_US.UTF-8
echo "127.0.0.1 ${HOSTNAME}" >> /etc/hosts

echo '[entrypoint] use ${DUNECONTROL} to configure and build dune-gdt'
echo

if [ "X$@" == "X" ]; then
    exec /bin/bash --rcfile <(cat /root/.bashrc; cat /venvs/${OPTS}.sh)
else
    exec /bin/bash -c ". /venvs/${OPTS}.sh && $@"
fi

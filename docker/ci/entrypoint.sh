#!/bin/bash

if [[ "X${OPTS}" == "X" ]] ; then
    >&2 echo "ERROR: need to specify one of deps/config.opts/ as OPTS"
    exit 1
fi

export LANG=en_US.UTF-8
echo "127.0.0.1 ${HOSTNAME}" >> /etc/hosts

echo '[entrypoint] ensure to append ${DCTL_ARGS} when calling dunecontrol'
echo

if [ "X$@" == "X" ]; then
    exec /bin/bash --rcfile <(cat /root/.bashrc; cat /venvs/venv-${OPTS}/bin/activate)
else
    exec /bin/bash -c ". /venvs/venv-${OPTS}/bin/activate && $@"
fi

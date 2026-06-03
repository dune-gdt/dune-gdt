#!/bin/bash

if [[ "X${OPTS}" == "X" ]] ; then
    >&2 echo "ERROR: need to specify one of deps/config.opts/ as OPTS"
    exit 1
fi

export LANG=en_US.UTF-8
echo "127.0.0.1 ${HOSTNAME}" >> /etc/hosts

export VENV_PREP_DIR="/venvs/${OPTS}/venv"
export VENV_TARGET_DIR="/build/${OPTS}/venv"

if [[ -e "${VENV_TARGET_DIR}" ]] ; then
    echo "[entrypoint] using existing venv in ${VENV_TARGET_DIR}"
    export VENV_SITE_PKG_PATH=$(. /venvs/${OPTS}.sh && python -c "import site; print(site.getsitepackages()[0])")
    for FILE in "${VENV_SITE_PKG_PATH}/dune.xt-nspkg.pth" "${VENV_SITE_PKG_PATH}/dune.gdt-nspkg.pth" ; do
        if [[ -e "${FILE}" ]] ; then
            echo "[entrypoint] ensuring short lines in ${FILE}"
            sed "s/;/\n/g" -i "${FILE}"
        fi
    done
else
    echo "[entrypoint] populating venv in ${VENV_TARGET_DIR}"
    cp -ar "${VENV_PREP_DIR}" "${VENV_TARGET_DIR}"
fi

echo '[entrypoint] use ${DUNECONTROL} to configure and build dune-gdt'
echo

git config --global --add safe.directory /source

if [ "X$@" == "X" ]; then
    exec /bin/bash --rcfile <(cat /root/.bashrc; cat /venvs/${OPTS}.sh)
else
    exec /bin/bash -c ". /venvs/${OPTS}.sh && $@"
fi

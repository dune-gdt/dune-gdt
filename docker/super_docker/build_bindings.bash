#!/bin/bash
for i in $(ls -d dune-xt* dune-gdt 2> /dev/null ); do
  cd ${SUPERDIR}/$i
  ${SRC_DCTRL} ${BLD} --only=${i} bexec ${BUILD_CMD} bindings || echo 'no bindings target'
done

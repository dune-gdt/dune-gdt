ARG BASE
FROM zivgitlab.wwu.io/ag-ohlberger/dune-community/docker/$BASE

ADD . ${SUPERDIR}/${MY_MODULE}/
RUN cd ${SUPERDIR} && \
    echo ${SRC_DCTRL} ${BLD} --only=${MY_MODULE} all && \
    ${SRC_DCTRL} ${BLD} --only=${MY_MODULE} all

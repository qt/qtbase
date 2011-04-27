TEMPLATE = subdirs
SUBDIRS = \
        access \
        kernel \
        ssl \
        socket

TRUSTED_BENCHMARKS += \
    kernel/qhostinfo \
    socket/qtcpserver \
    ssl/qsslsocket

include(../trusted-benchmarks.pri)
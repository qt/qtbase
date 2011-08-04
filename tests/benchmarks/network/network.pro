TEMPLATE = subdirs
SUBDIRS = \
        access \
        kernel \
        socket

TRUSTED_BENCHMARKS += \
    kernel/qhostinfo \
    socket/qtcpserver

contains(QT_CONFIG, openssl) {
   SUBDIRS += ssl
   TRUSTED_BENCHMARKS += ssl/qsslsocket
}

include(../trusted-benchmarks.pri)


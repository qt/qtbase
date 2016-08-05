TEMPLATE = subdirs
SUBDIRS = \
        access \
        kernel \
        socket

TRUSTED_BENCHMARKS += \
    kernel/qhostinfo \
    socket/qtcpserver

qtConfig(openssl) {
   SUBDIRS += ssl
   TRUSTED_BENCHMARKS += ssl/qsslsocket
}

include(../trusted-benchmarks.pri)


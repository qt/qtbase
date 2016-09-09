TEMPLATE = subdirs
QT_FOR_CONFIG += network-private

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


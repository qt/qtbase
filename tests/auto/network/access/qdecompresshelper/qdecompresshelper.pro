QT = network-private testlib
CONFIG += testcase parallel_test
TEMPLATE = app
TARGET = tst_qdecompresshelper

SOURCES += \
    tst_qdecompresshelper.cpp \
    gzip.rcc.cpp \
    inflate.rcc.cpp \

DEFINES += SRC_DIR="$$PWD"

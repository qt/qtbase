TEMPLATE = app
CONFIG += benchmark
CONFIG += exceptions
QT = core testlib

TARGET = tst_bench_qregexp
SOURCES += main.cpp
RESOURCES += qregexp.qrc

qtHaveModule(script):!pcre {
    DEFINES += HAVE_JSC
    QT += script
}

!qnx {
    exists($$[QT_SYSROOT]/usr/include/boost/regex.hpp) {
        DEFINES += HAVE_BOOST
        LIBS += -lboost_regex
    }
}

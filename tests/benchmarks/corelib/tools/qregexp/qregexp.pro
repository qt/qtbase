TEMPLATE = app
TARGET = tst_bench_qregexp
QT = core testlib
CONFIG += release exceptions

SOURCES += main.cpp
RESOURCES += qregexp.qrc

qtHaveModule(script):!pcre {
    DEFINES += HAVE_JSC
    QT += script
}

exists( /usr/include/boost/regex.hpp ){
DEFINES+=HAVE_BOOST
LIBS+=-lboost_regex
}

DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0

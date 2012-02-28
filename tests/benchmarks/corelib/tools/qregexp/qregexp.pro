TEMPLATE = app
TARGET = tst_bench_qregexp
QT = core testlib
CONFIG += release

SOURCES += main.cpp
RESOURCES += qregexp.qrc

!isEmpty(QT.webkit.sources):exists($${QT.webkit.sources}/../JavaScriptCore/JavaScriptCore.pri) {
    include( $${QT.webkit.sources}/../JavaScriptCore/JavaScriptCore.pri )
    DEFINES += HAVE_JSC
    QT += script
}

exists( /usr/include/boost/regex.hpp ){
DEFINES+=HAVE_BOOST
LIBS+=-lboost_regex
}


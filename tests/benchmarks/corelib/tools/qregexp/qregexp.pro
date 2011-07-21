load(qttest_p4)

# do not run benchmarks by default in 'make check'
CONFIG -= testcase

TEMPLATE = app
TARGET = tst_bench_qregexp
DEPENDPATH += .
INCLUDEPATH += .
RESOURCES+=qregexp.qrc
QT -= gui

CONFIG += release

# Input
SOURCES += main.cpp

!isEmpty(QT.webkit.sources):exists($${QT.webkit.sources}/../JavaScriptCore/JavaScriptCore.pri) {
    include( $${QT.webkit.sources}/../JavaScriptCore/JavaScriptCore.pri )
    DEFINES += HAVE_JSC
    QT += script
}

exists( /usr/include/boost/regex.hpp ){
DEFINES+=HAVE_BOOST
LIBS+=-lboost_regex
}


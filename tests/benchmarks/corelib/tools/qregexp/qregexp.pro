load(qttest_p4)
TEMPLATE = app
TARGET = tst_bench_qregexp
DEPENDPATH += .
INCLUDEPATH += .
RESOURCES+=qregexp.qrc
QT -= gui
QT += script

CONFIG += release

# Input
SOURCES += main.cpp

include( $${QT_SOURCE_TREE}/src/3rdparty/webkit/JavaScriptCore/JavaScriptCore.pri )

exists( /usr/include/boost/regex.hpp ){
DEFINES+=HAVE_BOOST
LIBS+=-lboost_regex
}


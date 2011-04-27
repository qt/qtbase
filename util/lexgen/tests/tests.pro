CONFIG += qtestlib
SOURCES += tst_lexgen.cpp
TARGET = tst_lexgen
include(../lexgen.pri)
QT = core
DEFINES += SRCDIR=\\\"$$PWD\\\"

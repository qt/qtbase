CONFIG += testcase
TARGET = tst_compiler
SOURCES += tst_compiler.cpp baseclass.cpp derivedclass.cpp
HEADERS += baseclass.h derivedclass.h
QT = core testlib



DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0

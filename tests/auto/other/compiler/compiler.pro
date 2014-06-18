CONFIG += testcase
TARGET = tst_compiler
SOURCES += tst_compiler.cpp baseclass.cpp derivedclass.cpp othersource.cpp
HEADERS += baseclass.h derivedclass.h
QT = core testlib
contains(QT_CONFIG, c++11): CONFIG += c++14 c++11


DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0

CONFIG += testcase
TARGET = tst_compiler
SOURCES += tst_compiler.cpp baseclass.cpp derivedclass.cpp othersource.cpp
HEADERS += baseclass.h derivedclass.h
QT = core testlib
qtConfig(c++11): CONFIG += c++11
qtConfig(c++14): CONFIG += c++14



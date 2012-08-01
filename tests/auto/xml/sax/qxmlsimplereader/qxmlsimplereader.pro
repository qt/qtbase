CONFIG += testcase
TARGET = tst_qxmlsimplereader
DEPENDPATH += parser
INCLUDEPATH += parser

HEADERS +=  parser/parser.h
SOURCES += tst_qxmlsimplereader.cpp parser/parser.cpp 

CONFIG += no_batch
QT += network xml testlib
QT -= gui

TESTDATA += encodings/* xmldocs/*
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0

CONFIG += testcase
TARGET = tst_qxmlsimplereader
INCLUDEPATH += parser

HEADERS +=  parser/parser.h
SOURCES += tst_qxmlsimplereader.cpp parser/parser.cpp 

CONFIG += no_batch
QT += network xml testlib
QT -= gui

TESTDATA += encodings/* xmldocs/*

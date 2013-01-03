CONFIG += testcase
CONFIG += parallel_test
TARGET = tst_qsyntaxhighlighter
SOURCES += tst_qsyntaxhighlighter.cpp
QT += testlib
qtHaveModule(widgets) QT += widgets
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0

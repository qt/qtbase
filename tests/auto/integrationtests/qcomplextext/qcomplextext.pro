CONFIG += testcase
TARGET = tst_qcomplextext
QT += widgets widgets-private testlib
QT += core-private gui-private
SOURCES  += tst_qcomplextext.cpp
INCLUDEPATH += $$QT_SOURCE_TREE/src/3rdparty/harfbuzz/src

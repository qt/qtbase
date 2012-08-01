CONFIG += testcase
TARGET = tst_qstandardpaths
QT = core testlib
SOURCES = tst_qstandardpaths.cpp
TESTDATA += tst_qstandardpaths.cpp qstandardpaths.pro
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0

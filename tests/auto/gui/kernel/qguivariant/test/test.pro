CONFIG += testcase
CONFIG += parallel_test
TARGET = tst_qguivariant
SOURCES  += tst_qguivariant.cpp
INCLUDEPATH += $$PWD/../../../../other/qvariant_common
QT += testlib
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0

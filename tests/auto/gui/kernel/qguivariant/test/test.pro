CONFIG += testcase
CONFIG += parallel_test
TARGET = tst_qguivariant
SOURCES  += tst_qguivariant.cpp
RESOURCES = tst_qguivariant.qrc
INCLUDEPATH += $$PWD/../../../../other/qvariant_common
QT += testlib
RESOURCES += qguivariant.qrc

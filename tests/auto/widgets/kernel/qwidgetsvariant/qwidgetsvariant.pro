CONFIG += testcase
CONFIG += parallel_test
TARGET = tst_qwidgetsvariant
INCLUDEPATH += $$PWD/../../../other/qvariant_common
SOURCES  += tst_qwidgetsvariant.cpp
QT += testlib widgets

DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0

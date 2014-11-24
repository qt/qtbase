CONFIG += testcase
CONFIG += parallel_test
TARGET = tst_qvariant
QT = core testlib
INCLUDEPATH += $$PWD/../../../other/qvariant_common
SOURCES = tst_qvariant.cpp
RESOURCES += qvariant.qrc
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
contains(QT_CONFIG, c++11): CONFIG += c++11

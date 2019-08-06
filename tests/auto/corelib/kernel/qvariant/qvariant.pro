CONFIG += testcase
TARGET = tst_qvariant
QT = core-private testlib
INCLUDEPATH += $$PWD/../../../other/qvariant_common
SOURCES = tst_qvariant.cpp
RESOURCES += qvariant.qrc
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
DEFINES -= QT_NO_LINKED_LIST
qtConfig(c++14): CONFIG += c++14
qtConfig(c++1z): CONFIG += c++1z
!qtConfig(doubleconversion):!qtConfig(system-doubleconversion) {
    DEFINES += QT_NO_DOUBLECONVERSION
}

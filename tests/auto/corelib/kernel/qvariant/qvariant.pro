CONFIG += testcase
TARGET = tst_qvariant
QT = core-private testlib
INCLUDEPATH += $$PWD/../../../other/qvariant_common
SOURCES = tst_qvariant.cpp
RESOURCES += qvariant.qrc
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
contains(QT_CONFIG, c++11): CONFIG += c++11
!contains(QT_CONFIG, doubleconversion):!contains(QT_CONFIG, system-doubleconversion) {
    DEFINES += QT_NO_DOUBLECONVERSION
}

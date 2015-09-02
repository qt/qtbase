CONFIG += testcase
TARGET = tst_qtimezone
QT = core-private testlib
SOURCES = tst_qtimezone.cpp
contains(QT_CONFIG,icu) {
    DEFINES += QT_USE_ICU
}

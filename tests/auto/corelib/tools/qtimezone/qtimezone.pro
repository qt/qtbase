CONFIG += testcase parallel_test
TARGET = tst_qtimezone
QT = core-private testlib
SOURCES = tst_qtimezone.cpp
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
contains(QT_CONFIG,icu) {
    DEFINES += QT_USE_ICU
}

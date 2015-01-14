CONFIG += testcase parallel_test
TARGET = tst_qstring
QT = core testlib
SOURCES = tst_qstring.cpp
DEFINES += QT_NO_CAST_TO_ASCII
contains(QT_CONFIG,icu):DEFINES += QT_USE_ICU
contains(QT_CONFIG,c++11): CONFIG += c++11
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0

mac {
    OBJECTIVE_SOURCES += tst_qstring_mac.mm
    LIBS += -framework Foundation
}

CONFIG += testcase
TARGET = tst_qtimezone
QT = core-private testlib
SOURCES = tst_qtimezone.cpp
qtConfig(icu) {
    DEFINES += QT_USE_ICU
}

darwin {
    OBJECTIVE_SOURCES += tst_qtimezone_darwin.mm
    LIBS += -framework Foundation
}

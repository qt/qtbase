CONFIG += testcase
TARGET = tst_qtimezone
QT = core-private testlib
SOURCES = tst_qtimezone.cpp
DEFINES += QT_NO_KEYWORDS QT_NO_FOREACH

qtConfig(icu): QMAKE_USE_PRIVATE += icu
darwin {
    OBJECTIVE_SOURCES += tst_qtimezone_darwin.mm
    LIBS += -framework Foundation
}

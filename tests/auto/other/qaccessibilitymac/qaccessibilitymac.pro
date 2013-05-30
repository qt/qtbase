CONFIG += testcase
TARGET = tst_qaccessibilitymac
# LIBS += -framework Carbon
QT += widgets testlib

HEADERS += tst_qaccessibilitymac_helpers.h
SOURCES += tst_qaccessibilitymac.cpp

mac {
    LIBS += -framework Security -framework AppKit -framework ApplicationServices
    OBJECTIVE_SOURCES += tst_qaccessibilitymac_helpers.mm
}


requires(mac)
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0

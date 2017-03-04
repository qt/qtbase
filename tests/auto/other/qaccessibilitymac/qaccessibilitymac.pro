CONFIG += testcase
TARGET = tst_qaccessibilitymac
QT += widgets testlib

HEADERS += tst_qaccessibilitymac_helpers.h
SOURCES += tst_qaccessibilitymac.cpp

mac {
    LIBS += -framework Security -framework AppKit -framework ApplicationServices
    OBJECTIVE_SOURCES += tst_qaccessibilitymac_helpers.mm
}


requires(mac)

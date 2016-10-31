CONFIG += testcase
TARGET = tst_qmenubar
QT += widgets testlib
SOURCES += tst_qmenubar.cpp

macos: {
    OBJECTIVE_SOURCES += tst_qmenubar_mac.mm
    LIBS += -framework AppKit
}

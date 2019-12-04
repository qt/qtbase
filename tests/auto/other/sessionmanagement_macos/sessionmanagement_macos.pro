CONFIG += testcase
TARGET = tst_sessionmanagement_macos

OBJECTIVE_SOURCES += tst_sessionmanagement_macos.mm

QT = testlib gui core
LIBS += -framework AppKit

requires(mac)
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0

CONFIG += testcase
testcase.timeout = 600 # this test is slow
TARGET = tst_qwidget

QT += widgets core-private gui-private widgets-private testlib testlib-private

SOURCES  += tst_qwidget.cpp
RESOURCES     = qwidget.qrc

aix-g++*:QMAKE_CXXFLAGS+=-fpermissive

CONFIG += x11inc

mac {
    LIBS += -framework Security -framework AppKit
    OBJECTIVE_SOURCES += tst_qwidget_mac_helpers.mm
}

win32:!winrt: LIBS += -luser32 -lgdi32

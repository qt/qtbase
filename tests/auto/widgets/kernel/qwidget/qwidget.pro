CONFIG += testcase
TARGET = tst_qwidget

QT += widgets core-private gui-private widgets-private testlib

SOURCES  += tst_qwidget.cpp
RESOURCES     = qwidget.qrc

aix-g++*:QMAKE_CXXFLAGS+=-fpermissive

CONFIG += x11inc

mac {
# ### fixme
# LIBS += -framework Security -framework AppKit -framework Carbon
# OBJECTIVE_SOURCES += tst_qwidget_mac_helpers.mm
}

x11 {
    LIBS += $$QMAKE_LIBS_X11
}

!wince*:win32: LIBS += -luser32 -lgdi32

CONFIG+=insignificant_test

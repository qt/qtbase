CONFIG += testcase
testcase.timeout = 600 # this test is slow
TARGET = tst_qwidget

QT += widgets core-private gui-private widgets-private testlib

SOURCES  += tst_qwidget.cpp
RESOURCES     = qwidget.qrc

aix-g++*:QMAKE_CXXFLAGS+=-fpermissive

CONFIG += x11inc

mac {
    LIBS += -framework Security -framework AppKit -framework Carbon
    OBJECTIVE_SOURCES += tst_qwidget_mac_helpers.mm
}

x11 {
    LIBS += $$QMAKE_LIBS_X11
}

!wince*:win32:!winrt: LIBS += -luser32 -lgdi32

mac:CONFIG+=insignificant_test # QTBUG-25300, QTBUG-23695
linux-*:system(". /etc/lsb-release && [ $DISTRIB_CODENAME = oneiric ]"):DEFINES+=UBUNTU_ONEIRIC # QTBUG-30566
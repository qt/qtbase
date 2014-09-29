CONFIG += testcase
CONFIG += parallel_test
TARGET = tst_qwidget_window
QT += widgets testlib core-private gui-private
SOURCES  += tst_qwidget_window.cpp

x11 {
    LIBS += $$QMAKE_LIBS_X11
}


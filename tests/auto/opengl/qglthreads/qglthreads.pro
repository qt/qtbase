CONFIG += testcase
TARGET = tst_qglthreads
requires(contains(QT_CONFIG,opengl))
QT += opengl widgets testlib

HEADERS += tst_qglthreads.h
SOURCES += tst_qglthreads.cpp

x11 {
    LIBS += $$QMAKE_LIBS_X11
}

CONFIG+=insignificant_test # QTBUG-22560
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0

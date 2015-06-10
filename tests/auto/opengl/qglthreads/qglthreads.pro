CONFIG += testcase
TARGET = tst_qglthreads
requires(qtHaveModule(opengl))
QT += opengl widgets testlib gui-private core-private

HEADERS += tst_qglthreads.h
SOURCES += tst_qglthreads.cpp

x11 {
    LIBS += $$QMAKE_LIBS_X11
}




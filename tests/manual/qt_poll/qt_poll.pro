CONFIG += testcase
TARGET = tst_qt_poll
QT = core-private network testlib
INCLUDEPATH += ../../../src/corelib/kernel
SOURCES += \
    tst_qt_poll.cpp \
    ../../../src/corelib/kernel/qpoll.cpp

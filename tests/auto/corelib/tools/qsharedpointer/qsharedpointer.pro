load(qttest_p4)

SOURCES += tst_qsharedpointer.cpp \
    forwarddeclaration.cpp \
    forwarddeclared.cpp \
    wrapper.cpp

HEADERS += forwarddeclared.h \
    wrapper.h

QT = core
DEFINES += SRCDIR=\\\"$$PWD/\\\"

include(externaltests.pri)
CONFIG += parallel_test

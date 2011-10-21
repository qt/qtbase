load(qt_module)

TARGET = QtTest
QPRO_PWD = $$PWD
QT = core

CONFIG += module
MODULE_PRI = ../modules/qt_testlib.pri

INCLUDEPATH += .
unix:!embedded:QMAKE_PKGCONFIG_DESCRIPTION = Qt \
    Unit \
    Testing \
    Library

# Input
HEADERS = qbenchmark.h \
    qsignalspy.h \
    qtestaccessible.h \
    qtestassert.h \
    qtestcase.h \
    qtestdata.h \
    qtestevent.h \
    qtesteventloop.h \
    qtest_global.h \
    qtest_gui.h \
    qtest.h \
    qtestkeyboard.h \
    qtestmouse.h \
    qtestspontaneevent.h \
    qtestsystem.h \
    qtesttouch.h \

SOURCES = qtestcase.cpp \
    qtestlog.cpp \
    qtesttable.cpp \
    qtestdata.cpp \
    qtestresult.cpp \
    qasciikey.cpp \
    qplaintestlogger.cpp \
    qxmltestlogger.cpp \
    qsignaldumper.cpp \
    qabstracttestlogger.cpp \
    qbenchmark.cpp \
    qbenchmarkmeasurement.cpp \
    qbenchmarkvalgrind.cpp \
    qbenchmarkevent.cpp \
    qbenchmarkmetric.cpp \
    qtestelement.cpp \
    qtestelementattribute.cpp \
    qtestxunitstreamer.cpp \
    qxunittestlogger.cpp
DEFINES *= QT_NO_CAST_TO_ASCII \
    QT_NO_CAST_FROM_ASCII \
    QTESTLIB_MAKEDLL \
    QT_NO_DATASTREAM
embedded:QMAKE_CXXFLAGS += -fno-rtti
wince*::LIBS += libcmt.lib \
    corelibc.lib \
    ole32.lib \
    oleaut32.lib \
    uuid.lib \
    commctrl.lib \
    coredll.lib \
    winsock.lib
mac:LIBS += -framework IOKit \
    -framework Security
!qpa:mac: LIBS += -framework ApplicationServices
qpa:mac: {
    contains(QT_CONFIG, coreservices) {
      LIBS_PRIVATE += -framework CoreServices
    } else {
      LIBS_PRIVATE += -framework CoreFoundation
    }
}

load(qt_module_config)

HEADERS += $$QT_SOURCE_TREE/src/testlib/qttestversion.h

QMAKE_TARGET_PRODUCT = QTestLib
QMAKE_TARGET_DESCRIPTION = Qt \
    Unit \
    Testing \
    Library

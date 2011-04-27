TARGET = QtTest
QPRO_PWD = $$PWD
QT = core
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
    qtestbasicstreamer.h \
    qtestcase.h \
    qtestcoreelement.h \
    qtestcorelist.h \
    qtestdata.h \
    qtestelementattribute.h \
    qtestelement.h \
    qtestevent.h \
    qtesteventloop.h \
    qtestfilelogger.h \
    qtest_global.h \
    qtest_gui.h \
    qtest.h \
    qtestkeyboard.h \
    qtestlightxmlstreamer.h \
    qtestmouse.h \
    qtestspontaneevent.h \
    qtestsystem.h \
    qtesttouch.h \
    qtestxmlstreamer.h \
    qtestxunitstreamer.h
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
    qtestbasicstreamer.cpp \
    qtestxunitstreamer.cpp \
    qtestxmlstreamer.cpp \
    qtestlightxmlstreamer.cpp \
    qtestlogger.cpp \
    qtestfilelogger.cpp
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

include(../qbase.pri)
QMAKE_TARGET_PRODUCT = QTestLib
QMAKE_TARGET_DESCRIPTION = Qt \
    Unit \
    Testing \
    Library
symbian:TARGET.UID3=0x2001B2DF

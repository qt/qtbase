TARGET = QtTest
QT = core-private
CONFIG += exceptions

MODULE_CONFIG = console testlib_defines

unix:!embedded:QMAKE_PKGCONFIG_DESCRIPTION = Qt \
    Unit \
    Testing \
    Library

QMAKE_DOCS = $$PWD/doc/qttestlib.qdocconf

HEADERS = qbenchmark.h \
    qbenchmark_p.h \
    qbenchmarkmeasurement_p.h \
    qbenchmarkvalgrind_p.h \
    qbenchmarkevent_p.h \
    qbenchmarkperfevents_p.h \
    qbenchmarkmetric.h \
    qbenchmarkmetric_p.h \
    qsignalspy.h \
    qtestaccessible.h \
    qtestassert.h \
    qtestcase.h \
    qtestdata.h \
    qtestevent.h \
    qtesteventloop.h \
    qtest_global.h \
    qtest_gui.h \
    qtest_widgets.h \
    qtest.h \
    qtestkeyboard.h \
    qtestmouse.h \
    qtestspontaneevent.h \
    qtestsystem.h \
    qtesttouch.h \
    qtestblacklist_p.h

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
    qbenchmarkperfevents.cpp \
    qbenchmarkmetric.cpp \
    qcsvbenchmarklogger.cpp \
    qtestelement.cpp \
    qtestelementattribute.cpp \
    qtestmouse.cpp \
    qtestxunitstreamer.cpp \
    qxunittestlogger.cpp \
    qtestblacklist.cpp

DEFINES *= QT_NO_CAST_TO_ASCII \
    QT_NO_CAST_FROM_ASCII \
    QT_NO_DATASTREAM
embedded:QMAKE_CXXFLAGS += -fno-rtti
wince: LIBS += \
    ole32.lib \
    oleaut32.lib \
    uuid.lib \
    commctrl.lib \
    coredll.lib \
    winsock.lib

mac {
    LIBS += -framework Security
    osx: LIBS += -framework ApplicationServices -framework IOKit

    # XCTest support
    !lessThan(QMAKE_XCODE_VERSION, "6.0") {
        OBJECTIVE_SOURCES += qxctestlogger.mm
        HEADERS += qxctestlogger_p.h

        DEFINES += HAVE_XCTEST
        LIBS += -framework Foundation

        load(sdk)
        platform_dev_frameworks_path = $${QMAKE_MAC_SDK_PLATFORM_PATH}/Developer/Library/Frameworks

        # We can't put this path into LIBS (so that it propagates to the prl file), as we
        # don't know yet if the target that links to testlib will build under Xcode or not.
        # The corresponding flags for the target lives in xctest.prf, where we do know.
        QMAKE_LFLAGS += -F$${platform_dev_frameworks_path} -weak_framework XCTest
        QMAKE_CXXFLAGS += -F$${platform_dev_frameworks_path}
        MODULE_CONFIG += xctest
    }
}

# Exclude these headers from the clean check if their dependencies aren't
# being built
!qtHaveModule(gui) {
    HEADERSCLEAN_EXCLUDE += qtest_gui.h \
        qtestaccessible.h \
        qtestkeyboard.h \
        qtestmouse.h \
        qtesttouch.h
}

!qtHaveModule(widgets): HEADERSCLEAN_EXCLUDE += qtest_widgets.h

load(qt_module)

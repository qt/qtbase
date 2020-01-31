TARGET = QtTest
QT = core-private
CONFIG += exceptions

MODULE_CONFIG = console testlib_defines

unix:!embedded:QMAKE_PKGCONFIG_DESCRIPTION = Qt \
    Unit \
    Testing \
    Library

QMAKE_DOCS = $$PWD/doc/qttestlib.qdocconf

HEADERS = \
    qabstracttestlogger_p.h \
    qbenchmark.h \
    qbenchmark_p.h \
    qbenchmarkmeasurement_p.h \
    qbenchmarktimemeasurers_p.h \
    qbenchmarkevent_p.h \
    qbenchmarkperfevents_p.h \
    qbenchmarkmetric.h \
    qbenchmarkmetric_p.h \
    qcsvbenchmarklogger_p.h \
    qplaintestlogger_p.h \
    qsignaldumper_p.h \
    qsignalspy.h \
    qteamcitylogger_p.h \
    qtestaccessible.h \
    qtestassert.h \
    qtestcase.h \
    qtestcoreelement_p.h \
    qtestcorelist_p.h \
    qtestdata.h \
    qtestevent.h \
    qtesteventloop.h \
    qtest_gui.h \
    qtest_network.h \
    qtest_widgets.h \
    qtest.h \
    qtestelement_p.h \
    qtestelementattribute_p.h \
    qtestkeyboard.h \
    qtestlog_p.h \
    qtestmouse.h \
    qtestresult_p.h \
    qtestspontaneevent.h \
    qtestsystem.h \
    qtesttable_p.h \
    qtesttouch.h \
    qtestblacklist_p.h \
    qtesthelpers_p.h \
    qttestglobal.h \
    qtestjunitstreamer_p.h \
    qtaptestlogger_p.h \
    qxmltestlogger_p.h \
    qjunittestlogger_p.h

SOURCES = \
    qtestcase.cpp \
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
    qbenchmarkevent.cpp \
    qbenchmarkperfevents.cpp \
    qbenchmarkmetric.cpp \
    qcsvbenchmarklogger.cpp \
    qteamcitylogger.cpp \
    qtestelement.cpp \
    qtestelementattribute.cpp \
    qtestmouse.cpp \
    qtestjunitstreamer.cpp \
    qjunittestlogger.cpp \
    qtestblacklist.cpp \
    qtaptestlogger.cpp

qtConfig(itemmodeltester) {
    HEADERS += \
        qabstractitemmodeltester.h

    SOURCES += \
        qabstractitemmodeltester.cpp
}

qtConfig(valgrind) {
    HEADERS += \
        qbenchmarkvalgrind_p.h
    SOURCES += \
        qbenchmarkvalgrind.cpp
}

DEFINES *= QT_NO_CAST_TO_ASCII \
    QT_NO_CAST_FROM_ASCII \
    QT_NO_FOREACH \
    QT_NO_DATASTREAM
embedded:QMAKE_CXXFLAGS += -fno-rtti

mac {
    LIBS += -framework Security

    SOURCES += qappletestlogger.cpp
    HEADERS += qappletestlogger_p.h

    macos {
        HEADERS += qtestutil_macos_p.h
        OBJECTIVE_SOURCES += qtestutil_macos.mm
        LIBS += -framework Foundation -framework ApplicationServices -framework IOKit -framework AppKit
    }

    # XCTest support (disabled for now)
    false:!lessThan(QMAKE_XCODE_VERSION, "6.0") {
        OBJECTIVE_SOURCES += qxctestlogger.mm
        HEADERS += qxctestlogger_p.h

        DEFINES += HAVE_XCTEST
        LIBS += -framework Foundation

        load(sdk)
        !isEmpty(QMAKE_MAC_SDK_PLATFORM_PATH) {
            platform_dev_frameworks_path = $${QMAKE_MAC_SDK_PLATFORM_PATH}/Developer/Library/Frameworks

            # We can't put this path into LIBS (so that it propagates to the prl file), as we
            # don't know yet if the target that links to testlib will build under Xcode or not.
            # The corresponding flags for the target lives in xctest.prf, where we do know.
            QMAKE_LFLAGS += -F$${platform_dev_frameworks_path} -weak_framework XCTest
            QMAKE_CXXFLAGS += -F$${platform_dev_frameworks_path}
            MODULE_CONFIG += xctest
        }
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

!qtHaveModule(network): HEADERSCLEAN_EXCLUDE += qtest_network.h

include(selfcover.pri)
load(qt_module)

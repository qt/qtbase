# Generated from testlib.pro.

#####################################################################
## Test Module:
#####################################################################

add_qt_module(Test
    CONFIG_MODULE_NAME testlib # special case
    SOURCES
        qabstracttestlogger.cpp
        qasciikey.cpp
        qbenchmark.cpp qbenchmark.h qbenchmark_p.h
        qbenchmarkevent.cpp qbenchmarkevent_p.h
        qbenchmarkmeasurement.cpp qbenchmarkmeasurement_p.h
        qbenchmarkmetric.cpp qbenchmarkmetric.h qbenchmarkmetric_p.h
        qbenchmarkperfevents.cpp qbenchmarkperfevents_p.h
        qbenchmarktimemeasurers_p.h
        qbenchmarkvalgrind.cpp qbenchmarkvalgrind_p.h
        qcsvbenchmarklogger.cpp
        qplaintestlogger.cpp
        qsignaldumper.cpp
        qsignalspy.h
        qtaptestlogger.cpp qtaptestlogger_p.h
        qteamcitylogger.cpp
        qtest.h
        qtest_gui.h
        qtest_network.h
        qtest_widgets.h
        qtestaccessible.h
        qtestassert.h
        qtestblacklist.cpp qtestblacklist_p.h
        qtestcase.cpp qtestcase.h
        qtestdata.cpp qtestdata.h
        qtestelement.cpp
        qtestelementattribute.cpp
        qtestevent.h
        qtesteventloop.h
        qtesthelpers_p.h
        qtestkeyboard.h
        qtestlog.cpp
        qtestmouse.cpp qtestmouse.h
        qtestresult.cpp
        qtestspontaneevent.h
        qtestsystem.h
        qtesttable.cpp
        qtesttouch.h
        qtestxunitstreamer.cpp
        qttestglobal.h
        qxmltestlogger.cpp
        qxunittestlogger.cpp
    DEFINES
        QT_NO_CAST_TO_ASCII
        QT_NO_CAST_FROM_ASCII
        QT_NO_FOREACH
        QT_NO_DATASTREAM
        QT_BUILD_TESTLIB_LIB # special case
    LIBRARIES
        Qt::CorePrivate
    PUBLIC_LIBRARIES # special case
        Qt::Core
)

#### Keys ignored in scope 1:.:testlib.pro:<NONE>:
# CONFIG = "exceptions"
# MODULE_CONFIG = "console" "testlib_defines"
# _LOADED = "qt_module"

## Scopes:
#####################################################################

#### Keys ignored in scope 2:.:testlib.pro:UNIX AND NOT embedded:
# QMAKE_PKGCONFIG_DESCRIPTION = "Qt" "Unit" "Testing" "Library"

extend_target(Test CONDITION QT_FEATURE_itemmodeltester
    SOURCES
        qabstractitemmodeltester.cpp qabstractitemmodeltester.h
)

extend_target(Test CONDITION embedded
    COMPILE_OPTIONS
        "-fno-rtti"
)

extend_target(Test CONDITION APPLE_OSX
    SOURCES
        qappletestlogger.cpp qappletestlogger_p.h
        qtestutil_macos.mm qtestutil_macos_p.h
    LIBRARIES
        ${FWApplicationServices}
        ${FWFoundation}
        ${FWIOKit}
        ${FWSecurity}
)

# special case: Do not bother with disabld stuff:
# extend_target(Test CONDITION (APPLE_OSX) AND (OFF AND NOT lessThan(QMAKE_XCODE_VERSION, "6.0")) ...
# extend_target(Test CONDITION ((APPLE_OSX) AND (OFF AND NOT lessThan(QMAKE_XCODE_VERSION, "6.0"))) AND (NOT QMAKE_MAC_SDK_PLATFORM_PATH_ISEMPTY) ...

#### Keys ignored in scope 8:.:testlib.pro:NOT QMAKE_MAC_SDK_PLATFORM_PATH_ISEMPTY:
# MODULE_CONFIG = "xctest"
# platform_dev_frameworks_path = "$${QMAKE_MAC_SDK_PLATFORM_PATH}/Developer/Library/Frameworks"

#### Keys ignored in scope 9:.:testlib.pro:NOT TARGET Qt::Gui:
# HEADERSCLEAN_EXCLUDE = "qtest_gui.h" "qtestaccessible.h" "qtestkeyboard.h" "qtestmouse.h" "qtesttouch.h"

#### Keys ignored in scope 10:.:testlib.pro:NOT TARGET Qt::Widgets:
# HEADERSCLEAN_EXCLUDE = "qtest_widgets.h"

#### Keys ignored in scope 11:.:testlib.pro:NOT TARGET Qt::Network:
# HEADERSCLEAN_EXCLUDE = "qtest_network.h"

add_qt_docs(./doc/qttestlib.qdocconf)

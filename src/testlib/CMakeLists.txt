# Generated from testlib.pro.

# special case begin
include(selfcover.cmake)
# special case end

#####################################################################
## Test Module:
#####################################################################

qt_internal_add_module(Test
    CONFIG_MODULE_NAME testlib
    QMAKE_MODULE_CONFIG console testlib_defines
    EXCEPTIONS
    SOURCES
        qabstracttestlogger.cpp qabstracttestlogger_p.h
        qasciikey.cpp
        qbenchmark.cpp qbenchmark.h qbenchmark_p.h
        qbenchmarkevent.cpp qbenchmarkevent_p.h
        qbenchmarkmeasurement.cpp qbenchmarkmeasurement_p.h
        qbenchmarkmetric.cpp qbenchmarkmetric.h qbenchmarkmetric_p.h
        qbenchmarkperfevents.cpp qbenchmarkperfevents_p.h
        qbenchmarktimemeasurers_p.h
        qcsvbenchmarklogger.cpp qcsvbenchmarklogger_p.h
        qemulationdetector_p.h
        qjunittestlogger.cpp qjunittestlogger_p.h
        qplaintestlogger.cpp qplaintestlogger_p.h
        qpropertytesthelper_p.h
        qsignaldumper.cpp qsignaldumper_p.h
        qsignalspy.h
        qtaptestlogger.cpp qtaptestlogger_p.h
        qteamcitylogger.cpp qteamcitylogger_p.h
        qtest.h
        qtest_gui.h
        qtest_network.h
        qtest_widgets.h
        qtestaccessible.h
        qtestassert.h
        qtestblacklist.cpp qtestblacklist_p.h
        qtestcase.cpp qtestcase.h
        qtestcoreelement_p.h
        qtestdata.cpp qtestdata.h
        qtestelement.cpp qtestelement_p.h
        qtestelementattribute.cpp qtestelementattribute_p.h
        qtestevent.h
        qtesteventloop.h
        qtesthelpers_p.h
        qtestjunitstreamer.cpp qtestjunitstreamer_p.h
        qtestkeyboard.h
        qtestlog.cpp qtestlog_p.h
        qtestmouse.cpp qtestmouse.h
        qtestresult.cpp qtestresult_p.h
        qtestspontaneevent.h
        qtestsystem.h
        qtesttable.cpp qtesttable_p.h
        qtesttouch.h
        qttestglobal.h
        qxmltestlogger.cpp qxmltestlogger_p.h
    DEFINES
        QT_NO_CAST_FROM_ASCII
        QT_NO_CAST_TO_ASCII
        QT_NO_DATASTREAM
        QT_NO_FOREACH
        # Ensure uniform location info between release and debug builds
        QT_NO_MESSAGELOGCONTEXT
    LIBRARIES
        Qt::CorePrivate
    PUBLIC_LIBRARIES
        Qt::Core
    PRIVATE_MODULE_INTERFACE
        Qt::CorePrivate
    GENERATE_CPP_EXPORTS
)

#### Keys ignored in scope 1:.:.:testlib.pro:<TRUE>:
# MODULE_CONFIG = "console" "testlib_defines"

## Scopes:
#####################################################################

#### Keys ignored in scope 2:.:.:testlib.pro:UNIX AND NOT embedded:
# QMAKE_PKGCONFIG_DESCRIPTION = "Qt" "Unit" "Testing" "Library"

qt_internal_extend_target(Test CONDITION QT_FEATURE_itemmodeltester
    SOURCES
        qabstractitemmodeltester.cpp qabstractitemmodeltester.h
)

qt_internal_extend_target(Test CONDITION QT_FEATURE_valgrind
    SOURCES
        qbenchmarkvalgrind.cpp qbenchmarkvalgrind_p.h
)

qt_internal_extend_target(Test CONDITION embedded
    COMPILE_OPTIONS
        -fno-rtti
)

qt_internal_extend_target(Test CONDITION APPLE
    SOURCES
        qappletestlogger.cpp qappletestlogger_p.h
    PUBLIC_LIBRARIES
        ${FWSecurity}
)

qt_internal_extend_target(Test CONDITION MACOS
    SOURCES
        qtestutil_macos.mm qtestutil_macos_p.h
    PUBLIC_LIBRARIES
        ${FWAppKit}
        ${FWApplicationServices}
        ${FWFoundation}
        ${FWIOKit}
)

set(qt_tc_build_dir "$<TARGET_PROPERTY:QT_TESTCASE_BUILDDIR>")
set(qt_bool_tc_build_dir "$<BOOL:${qt_tc_build_dir}>")
set(qt_tc_build_dir_def
    "$<IF:${qt_bool_tc_build_dir},${qt_tc_build_dir},$<TARGET_PROPERTY:BINARY_DIR>>"
)
set_property(TARGET Test APPEND PROPERTY INTERFACE_COMPILE_DEFINITIONS
    QT_TESTCASE_BUILDDIR="${qt_tc_build_dir_def}"
    QT_TESTCASE_SOURCEDIR="$<TARGET_PROPERTY:SOURCE_DIR>"
)

# special case begin
# Do not bother with disabled stuff:
# qt_internal_extend_target(Test CONDITION (MACOS) AND (OFF AND NOT lessThan(QMAKE_XCODE_VERSION, "6.0")) ...
# qt_internal_extend_target(Test CONDITION ((MACOS) AND (OFF AND NOT lessThan(QMAKE_XCODE_VERSION, "6.0"))) AND (NOT QMAKE_MAC_SDK_PLATFORM_PATH_ISEMPTY) ...
# special case end

#### Keys ignored in scope 9:.:.:testlib.pro:NOT QMAKE_MAC_SDK_PLATFORM_PATH_ISEMPTY:
# MODULE_CONFIG = "xctest"

#### Keys ignored in scope 10:.:.:testlib.pro:NOT TARGET Qt::Gui:
# HEADERSCLEAN_EXCLUDE = "qtest_gui.h" "qtestaccessible.h" "qtestkeyboard.h" "qtestmouse.h" "qtesttouch.h"

#### Keys ignored in scope 11:.:.:testlib.pro:NOT TARGET Qt::Widgets:
# HEADERSCLEAN_EXCLUDE = "qtest_widgets.h"

#### Keys ignored in scope 12:.:.:testlib.pro:NOT TARGET Qt::Network:
# HEADERSCLEAN_EXCLUDE = "qtest_network.h"
qt_internal_add_docs(Test
    doc/qttestlib.qdocconf
)

# special case begin
qt_internal_apply_testlib_coverage_options(Test)
# special case end

# include the snippet projects for developer-builds
if(QT_FEATURE_private_tests)
    add_subdirectory(doc/snippets/code)
endif()

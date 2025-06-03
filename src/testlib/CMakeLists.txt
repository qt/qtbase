# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

include(selfcover.cmake)

#####################################################################
## Test Module:
#####################################################################

include("${CMAKE_CURRENT_SOURCE_DIR}/QtTestProperties.cmake")

qt_internal_add_module(Test
    CONFIG_MODULE_NAME testlib
    QMAKE_MODULE_CONFIG console testlib_defines
    EXCEPTIONS
    SOURCES
        3rdparty/cycle/cycle_p.h # Needed by tests, so visible to syncqt.
        3rdparty/cycle/cycle_include_p.h # Also needed by tests.
        3rdparty/catch2/catch_p.h # Also needed by tests.
        removed_api.cpp     # keep first
        qabstracttestlogger.cpp qabstracttestlogger_p.h
        qasciikey.cpp
        qbenchmark.cpp qbenchmark.h qbenchmark_p.h
        qbenchmarkevent.cpp qbenchmarkevent_p.h
        qbenchmarkmeasurement.cpp qbenchmarkmeasurement_p.h
        qbenchmarkmetric.cpp qbenchmarkmetric.h qbenchmarkmetric_p.h
        qbenchmarkperfevents.cpp qbenchmarkperfevents_p.h
        qbenchmarktimemeasurers_p.h
        qcomparisontesthelper.cpp qcomparisontesthelper_p.h
        qcsvbenchmarklogger.cpp qcsvbenchmarklogger_p.h
        qemulationdetector_p.h
        qjunittestlogger.cpp qjunittestlogger_p.h
        qplaintestlogger.cpp qplaintestlogger_p.h
        qpropertytesthelper_p.h
        qsignaldumper.cpp qsignaldumper_p.h
        qsignalspy.cpp qsignalspy.h
        qtaptestlogger.cpp qtaptestlogger_p.h
        qteamcitylogger.cpp qteamcitylogger_p.h
        qtest.h
        qtest_gui.h
        qtest_network.h
        qtest_widgets.h
        qtestaccessible.h
        qtestassert.h
        qtestblacklist.cpp qtestblacklist_p.h
        qtestcase.cpp qtestcase.h qtestcase_p.h
        qtestcoreelement_p.h
        qtestcrashhandler_p.h
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
        qtesttostring.h
        qtestwheel.h
        qttestglobal.h
        qxmltestlogger.cpp qxmltestlogger_p.h
    DEFINES
        QT_NO_CAST_FROM_ASCII
        QT_NO_CAST_TO_ASCII
        QT_NO_CONTEXTLESS_CONNECT
        QT_NO_DATASTREAM
        QT_NO_FOREACH
        QT_USE_NODISCARD_FILE_OPEN
        # Ensure uniform location info between release and debug builds
        QT_NO_MESSAGELOGCONTEXT
    LIBRARIES
        Qt::CorePrivate
    NO_PCH_SOURCES
        removed_api.cpp
    PUBLIC_LIBRARIES
        Qt::Core
    PRIVATE_MODULE_INTERFACE
        Qt::CorePrivate
    ATTRIBUTION_FILE_DIR_PATHS
        3rdparty/catch2
        3rdparty/cycle
        3rdparty/linux
        3rdparty/valgrind
    EXTRA_CMAKE_FILES
        "${CMAKE_CURRENT_SOURCE_DIR}/QtTestProperties.cmake"
    EXTRA_CMAKE_INCLUDES
        QtTestProperties.cmake
)

if(TARGET Gui)
    set_property(TARGET Test
        APPEND PROPERTY _qt_internal_sync_headers_deps Gui_sync_headers)
endif()

## Scopes:
#####################################################################

qt_internal_extend_target(Test CONDITION QT_FEATURE_itemmodeltester
    SOURCES
        qabstractitemmodeltester.cpp qabstractitemmodeltester.h
)

qt_internal_extend_target(Test CONDITION QT_FEATURE_batch_test_support
    SOURCES
        qtestregistry.cpp qtestregistry_p.h
)

# 3rdparty/valgrind/*_p.h deliberately omitted, so syncqt doesn't
# mirror it in generated include/ directory - #include using "..._p.h"
# with a relative path. Only this module's sources and Core's qsimd.h
# should need to.
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

    # cycle_p.h expects the configure system to set up these defines.
    # Pass through COMPILE_OPTIONS instead of DEFINES, as we know
    # Clang supports defines with function macros on the command line.
    COMPILE_OPTIONS
        "-DHAVE_ARMV8_CNTVCT_EL0='defined(Q_PROCESSOR_ARM)'"
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

qt_internal_extend_target(Test CONDITION UNIX
    SOURCES
        qtestcrashhandler_unix.cpp
)

qt_internal_extend_target(Test CONDITION WIN32
    SOURCES
        qtestcrashhandler_win.cpp
)

set(qt_tc_build_dir "$<TARGET_PROPERTY:QT_TESTCASE_BUILDDIR>")
set(qt_bool_tc_build_dir "$<BOOL:${qt_tc_build_dir}>")
set(qt_tc_build_dir_def
    "$<IF:${qt_bool_tc_build_dir},${qt_tc_build_dir},$<TARGET_PROPERTY:BINARY_DIR>>"
)

string(JOIN "" testcase_dirs_genex
    "$<$<NOT:$<BOOL:$<TARGET_PROPERTY:QT_SKIP_DEFAULT_TESTCASE_DIRS>>>:"
        "QT_TESTCASE_BUILDDIR=\"${qt_tc_build_dir_def}\"$<SEMICOLON>"
        "QT_TESTCASE_SOURCEDIR=\"$<TARGET_PROPERTY:SOURCE_DIR>\""
    ">"
)
set_property(TARGET Test APPEND PROPERTY INTERFACE_COMPILE_DEFINITIONS
    ${testcase_dirs_genex}
)

qt_internal_add_docs(Test
    doc/qttestlib.qdocconf
)

qt_internal_apply_testlib_coverage_options(Test)

if(QT_FEATURE_doc_snippets)
    add_subdirectory(doc/snippets/code)
endif()

if(WASM)
    # Keep in sync with is_test files in Qt6WasmMacros.cmake
    set(testrunner_files
        "${QtBase_SOURCE_DIR}/util/wasm/batchedtestrunner/batchedtestrunner.html"
        "${QtBase_SOURCE_DIR}/util/wasm/batchedtestrunner/batchedtestrunner.js"
        "${QtBase_SOURCE_DIR}/util/wasm/batchedtestrunner/emrunadapter.js"
        "${QtBase_SOURCE_DIR}/util/wasm/batchedtestrunner/qwasmjsruntime.js"
        "${QtBase_SOURCE_DIR}/util/wasm/batchedtestrunner/qwasmtestmain.js"
        "${QtBase_SOURCE_DIR}/util/wasm/batchedtestrunner/util.js"
        "${QtBase_SOURCE_DIR}/util/wasm/batchedtestrunner/qtestoutputreporter.js"
        "${QtBase_SOURCE_DIR}/util/wasm/batchedtestrunner/qtestoutputreporter.css")

    qt_path_join(install_dir_libexec "${QT_INSTALL_DIR}" "${INSTALL_LIBEXECDIR}")
    qt_copy_or_install(FILES ${testrunner_files}
            DESTINATION "${install_dir_libexec}")

    # In a prefix build, the above copy_or_install won't put the files
    # in the build dir, but they are required there for tests at configure time
    if(QT_WILL_INSTALL)
        qt_path_join(build_dir_libexec "${QT_BUILD_DIR}" "${INSTALL_LIBEXECDIR}")
        foreach(testrunner_file ${testrunner_files})
            file(COPY "${testrunner_file}" DESTINATION "${build_dir_libexec}")
        endforeach()
    endif()
endif()

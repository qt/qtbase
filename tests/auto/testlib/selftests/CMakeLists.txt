# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# The whole file is written manually.

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_selftests LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

include(../../../../src/testlib/selfcover.cmake)

# ------------- Test runner -------------

# Resources:

file(GLOB qmake_expected_files_resource_files RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" "expected_*")
foreach(file IN LISTS qmake_expected_files_resource_files)
    set_source_files_properties("${CMAKE_CURRENT_SOURCE_DIR}/${file}" PROPERTIES QT_RESOURCE_ALIAS "${file}")
endforeach()

qt_internal_add_test(tst_selftests
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/"
    EXCEPTIONS
    CATCH
    SOURCES
        tst_selftests.cpp
        catch.cpp
    NO_PCH_SOURCES
        tst_selftests.cpp # undef QT_NO_FOREACH
    LIBRARIES
        Qt::TestPrivate
    TESTDATA ${qmake_expected_files_resource_files}
    BUILTIN_TESTDATA
    # These lines need to be commented out as they need to be enabled
    # COMPILE_OPTIONS
    #   conditionally
    #   --cs-exclude-file-abs-wildcard=${QT_SOURCE_TREE}/*
    #   --cs-include-file-abs-wildcard=*/src/testlib/*
    #   --cs-mcc
    #   --cs-mcdc
    # LINK_OPTIONS
    #   "$$COVERAGE_OPTIONS"
)

qt_internal_apply_testlib_coverage_options(tst_selftests)

# ------------- Sub tests -------------

set(subprograms
    assert
    badxml
    benchlibcounting
    benchlibeventcounter
    benchliboptions
    benchlibtickcounter
    benchlibwalltime
    blacklisted
    cmptest
    commandlinedata
    counting
    crashes
    datatable
    datetime
    deleteLater
    deleteLater_noApp
    differentexec
    eventloop
    exceptionthrow
    expectfail
    extendedcompare
    failcleanup
    failcleanuptestcase
    faildatatype
    failfetchtype
    failinit
    failinitdata
    fetchbogus
    findtestdata
    float
    globaldata
    junit
    longstring
    maxwarnings
    multiexec
    pass
    pairdiagnostics
    printdatatags
    printdatatagswithglobaltags
    qexecstringlist
    silent
    silent_fatal
    signaldumper
    singleskip
    skip
    skipblacklisted
    skipcleanup
    skipcleanuptestcase
    skipinit
    skipinitdata
    sleep
    strcmp
    subtest
    testlib
    tuplediagnostics
    verbose1
    verbose2
    verifyexceptionthrown
    warnings
    watchdog
)

if(NOT QT_FEATURE_sanitize_address)
    list(APPEND subprograms
        # valgrind and asan are incompatible:
        #   "ASan runtime does not come first in initial library list;"
        benchlibcallgrind
    )
endif()

if(FEATURE_cxx20)
  list(APPEND subprograms
      threewaycompare
  )
endif()

if(TARGET Qt::Gui)
    list(APPEND subprograms
        keyboard
        mouse
    )
endif()

# Ensure uniform location info between release and debug builds
add_compile_definitions(QT_MESSAGELOGCONTEXT)

foreach(subprogram IN LISTS subprograms)
    add_subdirectory(${subprogram})
    if(QT_FEATURE_process)
        add_dependencies(tst_selftests ${subprogram})
    endif()
endforeach()

list(JOIN subprograms " " subprograms)
qt_internal_extend_target(tst_selftests
    DEFINES
        $<$<COMPILE_LANGUAGE:CXX>:SUBPROGRAMS=${subprograms}>
)

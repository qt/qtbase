# The whole file is written manually.

include(../../../../src/testlib/selfcover.cmake)

# ------------- Test runner -------------

qt_internal_add_test(tst_selftests
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/"
    EXCEPTIONS
    CATCH
    SOURCES
        tst_selftests.cpp
        catch.cpp
    PUBLIC_LIBRARIES
        Qt::TestPrivate
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

file(GLOB resource_glob_0 RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" "expected_*")
foreach(file IN LISTS resource_glob_0)
    set_source_files_properties("${CMAKE_CURRENT_SOURCE_DIR}/${file}" PROPERTIES QT_RESOURCE_ALIAS "${file}")
endforeach()

# Resources:
set(qmake_expected_files_resource_files
    ${resource_glob_0}
)

qt_internal_add_resource(tst_selftests "qmake_expected_files"
    PREFIX
        "/"
    BASE
        "${CMAKE_CURRENT_SOURCE_DIR}"
    FILES
        ${qmake_expected_files_resource_files}
)

qt_internal_apply_testlib_coverage_options(tst_selftests)

# ------------- Sub tests -------------

set(subprograms
    assert
    badxml
    benchlibcallgrind
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
    failcleanup
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
    signaldumper
    singleskip
    skip
    skipcleanup
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

if(TARGET Qt::Gui)
    list(APPEND subprograms
        keyboard
        mouse
    )
endif()

# Ensure uniform location info between release and debug builds
add_definitions(-DQT_MESSAGELOGCONTEXT)

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

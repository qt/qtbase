# The whole file is written manually.
# special case skip regeneration

include(../../../../src/testlib/selfcover.cmake)

# ------------- Test runner -------------

qt_add_test(tst_selftests
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/"
    SOURCES
        ../../../shared/emulationdetector.h
        tst_selftests.cpp
    INCLUDE_DIRECTORIES
        ../../../shared
    PUBLIC_LIBRARIES
        Qt::TestPrivate
    # special case begin
    # These lines need to be commented out as they need to be enabled
    # COMPILE_OPTIONS
    #   conditionally
    #   --cs-exclude-file-abs-wildcard=${QT_SOURCE_TREE}/*
    #   --cs-include-file-abs-wildcard=*/src/testlib/*
    #   --cs-mcc
    #   --cs-mcdc
    # LINK_OPTIONS
    #   "$$COVERAGE_OPTIONS"
    # special case end
)

file(GLOB resource_glob_0 RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" "expected_*")
foreach(file IN LISTS resource_glob_0)
    set_source_files_properties("${CMAKE_CURRENT_SOURCE_DIR}/${file}" PROPERTIES QT_RESOURCE_ALIAS "${file}")
endforeach()

# Resources:
set(qmake_expected_files_resource_files
    ${resource_glob_0}
)

qt_add_resource(tst_selftests "qmake_expected_files"
    PREFIX
        "/"
    BASE
        "${CMAKE_CURRENT_SOURCE_DIR}" # special case
    FILES
        ${qmake_expected_files_resource_files}
)


## Scopes:
#####################################################################

#### Keys ignored in scope 5:.:.:test.pro:NOT ANDROID AND NOT WINRT AND for(file,SUBPROGRAMS):
# TEST_HELPER_INSTALLS = "../$${file}/$${file}"

#### Keys ignored in scope 7:.:..:../selftests.pri:TARGET Qt::Gui:
# SUBPROGRAMS = "keyboard" "mouse"

qt_apply_testlib_coverage_options(tst_selftests)

# ------------- Sub tests -------------

if(NOT WINRT)
    add_subdirectory(assert)
    add_subdirectory(badxml)
    add_subdirectory(benchlibcallgrind)
    add_subdirectory(benchlibcounting)
    add_subdirectory(benchlibeventcounter)
    add_subdirectory(benchliboptions)
    add_subdirectory(benchlibtickcounter)
    add_subdirectory(benchlibwalltime)
    add_subdirectory(blacklisted)
    add_subdirectory(cmptest)
    add_subdirectory(commandlinedata)
    add_subdirectory(counting)
    add_subdirectory(crashes)
    add_subdirectory(datatable)
    add_subdirectory(datetime)
    add_subdirectory(deleteLater)
    add_subdirectory(deleteLater_noApp)
    add_subdirectory(differentexec)
    add_subdirectory(exceptionthrow)
    add_subdirectory(expectfail)
    add_subdirectory(failcleanup)
    add_subdirectory(faildatatype)
    add_subdirectory(failfetchtype)
    add_subdirectory(failinit)
    add_subdirectory(failinitdata)
    add_subdirectory(fetchbogus)
    add_subdirectory(findtestdata)
    add_subdirectory(float)
    add_subdirectory(globaldata)
    add_subdirectory(longstring)
    add_subdirectory(maxwarnings)
    add_subdirectory(multiexec)
    add_subdirectory(pass)
    add_subdirectory(pairdiagnostics)
    add_subdirectory(printdatatags)
    add_subdirectory(printdatatagswithglobaltags)
    add_subdirectory(qexecstringlist)
    add_subdirectory(silent)
    add_subdirectory(signaldumper)
    add_subdirectory(singleskip)
    add_subdirectory(skip)
    add_subdirectory(skipcleanup)
    add_subdirectory(skipinit)
    add_subdirectory(skipinitdata)
    add_subdirectory(sleep)
    add_subdirectory(strcmp)
    add_subdirectory(subtest)
    add_subdirectory(testlib)
    add_subdirectory(tuplediagnostics)
    add_subdirectory(verbose1)
    add_subdirectory(verbose2)
    add_subdirectory(verifyexceptionthrown)
    add_subdirectory(warnings)
    add_subdirectory(watchdog)
    add_subdirectory(xunit)

    if(TARGET Qt::Gui)
        add_subdirectory(keyboard)
        add_subdirectory(mouse)
    endif()
endif()

#### Keys ignored in scope 1:.:.:test.pro:<TRUE>:
# CONFIG = "testcase"
# expected_files.base = "$$PWD/.."
# expected_files.files = "$$files$$PWD/../expected_*"

## Scopes:
#####################################################################

#### Keys ignored in scope 5:.:.:test.pro:NOT ANDROID AND NOT WINRT AND for(file,SUBPROGRAMS):
# TEST_HELPER_INSTALLS = "../$${file}/$${file}"
# special case end

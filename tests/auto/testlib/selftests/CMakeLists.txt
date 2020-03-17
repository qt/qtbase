# Generated from selftests.pro.

# special case begin
include(../../../../src/testlib/selfcover.cmake)
# special case end

add_subdirectory(test)

if(NOT WINRT)

    if(TARGET Qt::Gui)
    endif()
endif()
# special case begin

# The whole file is written manually.

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

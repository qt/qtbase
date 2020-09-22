# Generated from benchlibcounting.pro.

#####################################################################
## benchlibcounting Binary:
#####################################################################

qt_internal_add_executable(benchlibcounting
    NO_INSTALL # special case
    OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} # special case
    SOURCES
        tst_benchlibcounting.cpp
    PUBLIC_LIBRARIES
        Qt::Test
)

## Scopes:
#####################################################################

# special case begin
qt_internal_apply_testlib_coverage_options(tst_selftests)
# special case end

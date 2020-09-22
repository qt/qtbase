# Generated from benchlibcallgrind.pro.

#####################################################################
## benchlibcallgrind Binary:
#####################################################################

qt_internal_add_executable(benchlibcallgrind
    NO_INSTALL # special case
    OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} # special case
    SOURCES
        tst_benchlibcallgrind.cpp
    PUBLIC_LIBRARIES
        Qt::Test
)

## Scopes:
#####################################################################

# special case begin
qt_internal_apply_testlib_coverage_options(benchlibcallgrind)
# special case end

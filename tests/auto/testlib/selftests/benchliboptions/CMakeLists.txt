# Generated from benchliboptions.pro.

#####################################################################
## benchliboptions Binary:
#####################################################################

qt_internal_add_executable(benchliboptions
    NO_INSTALL # special case
    OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} # special case
    SOURCES
        tst_benchliboptions.cpp
    PUBLIC_LIBRARIES
        Qt::Test
)

## Scopes:
#####################################################################

# special case begin
qt_internal_apply_testlib_coverage_options(tst_selftests)
# special case end

# Generated from benchlibeventcounter.pro.

#####################################################################
## benchlibeventcounter Binary:
#####################################################################

qt_internal_add_executable(benchlibeventcounter
    NO_INSTALL # special case
    OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} # special case
    SOURCES
        tst_benchlibeventcounter.cpp
    PUBLIC_LIBRARIES
        Qt::Test
)

## Scopes:
#####################################################################

# special case begin
qt_internal_apply_testlib_coverage_options(benchlibeventcounter)
# special case end

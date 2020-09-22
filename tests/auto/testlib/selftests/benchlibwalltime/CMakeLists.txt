# Generated from benchlibwalltime.pro.

#####################################################################
## benchlibwalltime Binary:
#####################################################################

qt_internal_add_executable(benchlibwalltime
    NO_INSTALL # special case
    OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} # special case
    SOURCES
        tst_benchlibwalltime.cpp
    PUBLIC_LIBRARIES
        Qt::Test
)

## Scopes:
#####################################################################

# special case begin
qt_internal_apply_testlib_coverage_options(benchlibwalltime)
# special case end

# Generated from benchlibtickcounter.pro.

#####################################################################
## benchlibtickcounter Binary:
#####################################################################

qt_internal_add_executable(benchlibtickcounter
    NO_INSTALL # special case
    OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} # special case
    SOURCES
        tst_benchlibtickcounter.cpp
    PUBLIC_LIBRARIES
        Qt::TestPrivate
)

## Scopes:
#####################################################################

# special case begin
qt_internal_apply_testlib_coverage_options(benchlibtickcounter)
# special case end

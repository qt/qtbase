# Generated from xunit.pro.

#####################################################################
## xunit Binary:
#####################################################################

qt_internal_add_executable(xunit
    NO_INSTALL # special case
    OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} # special case
    SOURCES
        tst_xunit.cpp
    PUBLIC_LIBRARIES
        Qt::Test
)

## Scopes:
#####################################################################

# special case begin
qt_internal_apply_testlib_coverage_options(xunit)
# special case end

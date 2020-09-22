# Generated from subtest.pro.

#####################################################################
## subtest Binary:
#####################################################################

qt_internal_add_executable(subtest
    NO_INSTALL # special case
    OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} # special case
    SOURCES
        tst_subtest.cpp
    PUBLIC_LIBRARIES
        Qt::Test
)

## Scopes:
#####################################################################

# special case begin
qt_internal_apply_testlib_coverage_options(subtest)
# special case end

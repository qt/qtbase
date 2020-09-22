# Generated from failcleanup.pro.

#####################################################################
## failcleanup Binary:
#####################################################################

qt_internal_add_executable(failcleanup
    NO_INSTALL # special case
    OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} # special case
    SOURCES
        tst_failcleanup.cpp
    PUBLIC_LIBRARIES
        Qt::Test
)

## Scopes:
#####################################################################

# special case begin
qt_internal_apply_testlib_coverage_options(failcleanup)
# special case end

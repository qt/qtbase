# Generated from sleep.pro.

#####################################################################
## sleep Binary:
#####################################################################

qt_internal_add_executable(sleep
    NO_INSTALL # special case
    OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} # special case
    SOURCES
        tst_sleep.cpp
    PUBLIC_LIBRARIES
        Qt::Test
)

## Scopes:
#####################################################################

# special case begin
qt_internal_apply_testlib_coverage_options(sleep)
# special case end

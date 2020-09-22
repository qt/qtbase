# Generated from crashes.pro.

#####################################################################
## crashes Binary:
#####################################################################

qt_internal_add_executable(crashes
    NO_INSTALL # special case
    OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} # special case
    SOURCES
        tst_crashes.cpp
    PUBLIC_LIBRARIES
        Qt::Test
)

## Scopes:
#####################################################################

# special case begin
qt_internal_apply_testlib_coverage_options(crashes)
# special case end

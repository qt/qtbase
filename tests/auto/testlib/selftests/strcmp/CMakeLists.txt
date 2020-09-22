# Generated from strcmp.pro.

#####################################################################
## strcmp Binary:
#####################################################################

qt_internal_add_executable(strcmp
    NO_INSTALL # special case
    OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} # special case
    SOURCES
        tst_strcmp.cpp
    PUBLIC_LIBRARIES
        Qt::Test
)

## Scopes:
#####################################################################

# special case begin
qt_internal_apply_testlib_coverage_options(strcmp)
# special case end

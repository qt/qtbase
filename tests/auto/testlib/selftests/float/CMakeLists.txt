# Generated from float.pro.

#####################################################################
## float Binary:
#####################################################################

qt_internal_add_executable(float
    NO_INSTALL # special case
    OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} # special case
    SOURCES
        tst_float.cpp
    PUBLIC_LIBRARIES
        Qt::Test
        Qt::TestPrivate
)

## Scopes:
#####################################################################

# special case begin
qt_internal_apply_testlib_coverage_options(float)
# special case end

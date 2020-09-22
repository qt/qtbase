# Generated from assert.pro.

#####################################################################
## assert Binary:
#####################################################################

qt_internal_add_executable(assert
    NO_INSTALL # special case
    OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} # special case
    SOURCES
        tst_assert.cpp
    PUBLIC_LIBRARIES
        Qt::Test
)

## Scopes:
#####################################################################

# special case begin
qt_internal_apply_testlib_coverage_options(assert)
# special case end

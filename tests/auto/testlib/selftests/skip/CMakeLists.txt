# Generated from skip.pro.

#####################################################################
## skip Binary:
#####################################################################

qt_internal_add_executable(skip
    NO_INSTALL # special case
    OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} # special case
    SOURCES
        tst_skip.cpp
    PUBLIC_LIBRARIES
        Qt::Test
)

## Scopes:
#####################################################################

# special case begin
qt_internal_apply_testlib_coverage_options(skip)
# special case end

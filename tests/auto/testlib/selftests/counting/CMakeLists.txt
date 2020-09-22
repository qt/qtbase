# Generated from counting.pro.

#####################################################################
## counting Binary:
#####################################################################

qt_internal_add_executable(counting
    NO_INSTALL # special case
    OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} # special case
    SOURCES
        tst_counting.cpp
    PUBLIC_LIBRARIES
        Qt::Test
)

## Scopes:
#####################################################################

# special case begin
qt_internal_apply_testlib_coverage_options(counting)
# special case end

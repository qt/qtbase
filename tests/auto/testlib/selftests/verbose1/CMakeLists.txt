# Generated from verbose1.pro.

#####################################################################
## verbose1 Binary:
#####################################################################

qt_internal_add_executable(verbose1
    NO_INSTALL # special case
    OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} # special case
    SOURCES
        ../counting/tst_counting.cpp
    DEFINES
        TESTLIB_VERBOSITY_ARG=-v1 # special case remove quotes
    PUBLIC_LIBRARIES
        Qt::Test
)

## Scopes:
#####################################################################

# special case begin
qt_internal_apply_testlib_coverage_options(verbose1)
# special case end

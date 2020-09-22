# Generated from singleskip.pro.

#####################################################################
## singleskip Binary:
#####################################################################

qt_internal_add_executable(singleskip
    NO_INSTALL # special case
    OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} # special case
    SOURCES
        tst_singleskip.cpp
    PUBLIC_LIBRARIES
        Qt::Test
)

## Scopes:
#####################################################################

# special case begin
qt_internal_apply_testlib_coverage_options(singleskip)
# special case end

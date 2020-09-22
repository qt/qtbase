# Generated from printdatatags.pro.

#####################################################################
## printdatatags Binary:
#####################################################################

qt_internal_add_executable(printdatatags
    NO_INSTALL # special case
    OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} # special case
    SOURCES
        tst_printdatatags.cpp
    PUBLIC_LIBRARIES
        Qt::Test
)

## Scopes:
#####################################################################

# special case begin
qt_internal_apply_testlib_coverage_options(printdatatags)
# special case end

# Generated from commandlinedata.pro.

#####################################################################
## commandlinedata Binary:
#####################################################################

qt_add_executable(commandlinedata
    NO_INSTALL # special case
    OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} # special case
    SOURCES
        tst_commandlinedata.cpp
    PUBLIC_LIBRARIES
        Qt::Test
)

## Scopes:
#####################################################################

# special case begin
qt_apply_testlib_coverage_options(commandlinedata)
# special case end

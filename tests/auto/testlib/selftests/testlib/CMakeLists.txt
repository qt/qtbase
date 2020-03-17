# Generated from testlib.pro.

#####################################################################
## testlib Binary:
#####################################################################

add_qt_executable(testlib
    NO_INSTALL # special case
    OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} # special case
    SOURCES
        tst_testlib.cpp
    PUBLIC_LIBRARIES
        Qt::Test
)

## Scopes:
#####################################################################

# special case begin
qt_apply_testlib_coverage_options(testlib)
# special case end

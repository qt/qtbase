# Generated from expectfail.pro.

#####################################################################
## expectfail Binary:
#####################################################################

add_qt_executable(expectfail
    NO_INSTALL # special case
    OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} # special case
    SOURCES
        tst_expectfail.cpp
    PUBLIC_LIBRARIES
        Qt::Test
)

## Scopes:
#####################################################################

# special case begin
qt_apply_testlib_coverage_options(expectfail)
# special case end

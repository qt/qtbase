# Generated from longstring.pro.

#####################################################################
## longstring Binary:
#####################################################################

add_qt_executable(longstring
    NO_INSTALL # special case
    OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} # special case
    SOURCES
        tst_longstring.cpp
    PUBLIC_LIBRARIES
        Qt::Test
)

## Scopes:
#####################################################################

# special case begin
qt_apply_testlib_coverage_options(longstring)
# special case end

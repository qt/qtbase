# Generated from maxwarnings.pro.

#####################################################################
## maxwarnings Binary:
#####################################################################

add_qt_executable(maxwarnings
    NO_INSTALL # special case
    OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} # special case
    SOURCES
        maxwarnings.cpp
    PUBLIC_LIBRARIES
        Qt::Test
)

## Scopes:
#####################################################################

# special case begin
qt_apply_testlib_coverage_options(maxwarnings)
# special case end

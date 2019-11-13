# Generated from xunit.pro.

#####################################################################
## xunit Binary:
#####################################################################

add_qt_executable(xunit
    NO_INSTALL # special case
    OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} # special case
    SOURCES
        tst_xunit.cpp
    PUBLIC_LIBRARIES
        Qt::Test
)

## Scopes:
#####################################################################

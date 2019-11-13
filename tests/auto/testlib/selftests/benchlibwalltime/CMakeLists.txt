# Generated from benchlibwalltime.pro.

#####################################################################
## benchlibwalltime Binary:
#####################################################################

add_qt_executable(benchlibwalltime
    NO_INSTALL # special case
    OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} # special case
    SOURCES
        tst_benchlibwalltime.cpp
    PUBLIC_LIBRARIES
        Qt::Test
)

## Scopes:
#####################################################################

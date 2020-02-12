# Generated from benchlibtickcounter.pro.

#####################################################################
## benchlibtickcounter Binary:
#####################################################################

add_qt_executable(benchlibtickcounter
    NO_INSTALL # special case
    OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} # special case
    GUI
    SOURCES
        tst_benchlibtickcounter.cpp
    PUBLIC_LIBRARIES
        Qt::TestPrivate
)

## Scopes:
#####################################################################

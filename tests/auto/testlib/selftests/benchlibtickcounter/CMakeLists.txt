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
    LIBRARIES
        Qt::TestPrivate
    PUBLIC_LIBRARIES
        Qt::Test
)

#### Keys ignored in scope 1:.:.:benchlibtickcounter.pro:<TRUE>:
# CONFIG = "-debug_and_release_target"

## Scopes:
#####################################################################

#### Keys ignored in scope 2:.:.:benchlibtickcounter.pro:APPLE_OSX:
# CONFIG = "-app_bundle"

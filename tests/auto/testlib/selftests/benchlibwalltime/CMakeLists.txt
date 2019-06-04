# Generated from benchlibwalltime.pro.

#####################################################################
## benchlibwalltime Binary:
#####################################################################

add_qt_executable(benchlibwalltime
    NO_INSTALL # special case
    OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} # special case
    GUI
    SOURCES
        tst_benchlibwalltime.cpp
    PUBLIC_LIBRARIES
        Qt::Test
)

#### Keys ignored in scope 1:.:.:benchlibwalltime.pro:<TRUE>:
# CONFIG = "-debug_and_release_target"

## Scopes:
#####################################################################

#### Keys ignored in scope 2:.:.:benchlibwalltime.pro:APPLE_OSX:
# CONFIG = "-app_bundle"

# Generated from sleep.pro.

#####################################################################
## sleep Binary:
#####################################################################

add_qt_executable(sleep
    NO_INSTALL # special case
    OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} # special case
    GUI
    SOURCES
        tst_sleep.cpp
    PUBLIC_LIBRARIES
        Qt::Test
)

#### Keys ignored in scope 1:.:.:sleep.pro:<TRUE>:
# CONFIG = "-debug_and_release_target"

## Scopes:
#####################################################################

#### Keys ignored in scope 2:.:.:sleep.pro:APPLE_OSX:
# CONFIG = "-app_bundle"

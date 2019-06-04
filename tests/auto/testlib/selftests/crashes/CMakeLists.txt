# Generated from crashes.pro.

#####################################################################
## crashes Binary:
#####################################################################

add_qt_executable(crashes
    NO_INSTALL # special case
    OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} # special case
    GUI
    SOURCES
        tst_crashes.cpp
    PUBLIC_LIBRARIES
        Qt::Test
)

#### Keys ignored in scope 1:.:.:crashes.pro:<TRUE>:
# CONFIG = "-debug_and_release_target"

## Scopes:
#####################################################################

#### Keys ignored in scope 2:.:.:crashes.pro:APPLE_OSX:
# CONFIG = "-app_bundle"

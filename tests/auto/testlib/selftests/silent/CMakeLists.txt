# Generated from silent.pro.

#####################################################################
## silent Binary:
#####################################################################

add_qt_executable(silent
    NO_INSTALL # special case
    OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} # special case
    GUI
    SOURCES
        tst_silent.cpp
    LIBRARIES
        Qt::TestPrivate
    PUBLIC_LIBRARIES
        Qt::Test
)

#### Keys ignored in scope 1:.:.:silent.pro:<TRUE>:
# CONFIG = "-debug_and_release_target"

## Scopes:
#####################################################################

#### Keys ignored in scope 2:.:.:silent.pro:APPLE_OSX:
# CONFIG = "-app_bundle"

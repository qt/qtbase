# Generated from keyboard.pro.

#####################################################################
## keyboard Binary:
#####################################################################

add_qt_executable(keyboard
    NO_INSTALL # special case
    OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} # special case
    GUI
    SOURCES
        tst_keyboard.cpp
    LIBRARIES
        Qt::GuiPrivate
        Qt::TestPrivate
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::Test
)

#### Keys ignored in scope 1:.:.:keyboard.pro:<TRUE>:
# CONFIG = "-debug_and_release_target"

## Scopes:
#####################################################################

#### Keys ignored in scope 2:.:.:keyboard.pro:APPLE_OSX:
# CONFIG = "-app_bundle"

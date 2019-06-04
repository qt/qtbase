# Generated from mouse.pro.

#####################################################################
## mouse Binary:
#####################################################################

add_qt_executable(mouse
    NO_INSTALL # special case
    OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} # special case
    GUI
    SOURCES
        tst_mouse.cpp
    LIBRARIES
        Qt::GuiPrivate
        Qt::TestPrivate
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::Test
)

#### Keys ignored in scope 1:.:.:mouse.pro:<TRUE>:
# CONFIG = "-debug_and_release_target"

## Scopes:
#####################################################################

#### Keys ignored in scope 2:.:.:mouse.pro:APPLE_OSX:
# CONFIG = "-app_bundle"

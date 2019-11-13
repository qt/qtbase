# Generated from keyboard.pro.

#####################################################################
## keyboard Binary:
#####################################################################

add_qt_executable(keyboard
    NO_INSTALL # special case
    OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} # special case
    SOURCES
        tst_keyboard.cpp
    LIBRARIES
        Qt::GuiPrivate
        Qt::TestPrivate
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::Test
)

## Scopes:
#####################################################################

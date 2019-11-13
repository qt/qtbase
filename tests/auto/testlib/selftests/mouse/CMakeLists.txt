# Generated from mouse.pro.

#####################################################################
## mouse Binary:
#####################################################################

add_qt_executable(mouse
    NO_INSTALL # special case
    OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} # special case
    SOURCES
        tst_mouse.cpp
    LIBRARIES
        Qt::GuiPrivate
        Qt::TestPrivate
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::Test
)

## Scopes:
#####################################################################

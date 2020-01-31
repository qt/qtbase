# Generated from flicker.pro.

#####################################################################
## flicker Binary:
#####################################################################

add_qt_manual_test(flicker
    GUI
    SOURCES
        main.cpp
        window.cpp window.h
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::Widgets
)

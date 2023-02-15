
#####################################################################
## screengadget Binary:
#####################################################################

qt_internal_add_manual_test(screengadget
    GUI
    SOURCES
        main.cpp
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::Widgets
        Qt::GuiPrivate
)

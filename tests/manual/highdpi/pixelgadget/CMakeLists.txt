
#####################################################################
## pixelgadget Binary:
#####################################################################

qt_internal_add_manual_test(pixelgadget
    GUI
    SOURCES
        main.cpp
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::Widgets
)

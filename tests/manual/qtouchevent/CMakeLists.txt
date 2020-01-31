# Generated from qtouchevent.pro.

#####################################################################
## qtouchevent Binary:
#####################################################################

add_qt_manual_test(qtouchevent
    SOURCES
        form.ui
        main.cpp
        touchwidget.cpp touchwidget.h
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::Test
        Qt::Widgets
    ENABLE_AUTOGEN_TOOLS
        uic
)

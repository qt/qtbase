# Generated from graphicsview.pro.

#####################################################################
## tst_manual_graphicsview Binary:
#####################################################################

add_qt_manual_test(tst_manual_graphicsview
    GUI
    SOURCES
        gestures.cpp gestures.h
        imageitem.cpp imageitem.h
        main.cpp
        mousepangesturerecognizer.cpp mousepangesturerecognizer.h
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::Widgets
)

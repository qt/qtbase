# Generated from qtabwidget.pro.

#####################################################################
## tst_qtabwidget Test:
#####################################################################

add_qt_test(tst_qtabwidget
    SOURCES
        tst_qtabwidget.cpp
    INCLUDE_DIRECTORIES
        ..
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::Widgets
        Qt::WidgetsPrivate
)

## Scopes:
#####################################################################

extend_target(tst_qtabwidget CONDITION WIN32 AND NOT WINRT
    PUBLIC_LIBRARIES
        user32
)

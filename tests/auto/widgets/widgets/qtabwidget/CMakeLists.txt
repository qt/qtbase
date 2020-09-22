# Generated from qtabwidget.pro.

#####################################################################
## tst_qtabwidget Test:
#####################################################################

qt_internal_add_test(tst_qtabwidget
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

qt_internal_extend_target(tst_qtabwidget CONDITION WIN32
    PUBLIC_LIBRARIES
        user32
)

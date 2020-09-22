# Generated from qwindow.pro.

#####################################################################
## tst_qwindow Test:
#####################################################################

qt_internal_add_test(tst_qwindow
    SOURCES
        tst_qwindow.cpp
    PUBLIC_LIBRARIES
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
)

## Scopes:
#####################################################################

qt_internal_extend_target(tst_qwindow CONDITION QT_FEATURE_dynamicgl AND WIN32
    PUBLIC_LIBRARIES
        user32
)

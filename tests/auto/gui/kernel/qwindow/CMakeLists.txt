# Generated from qwindow.pro.

#####################################################################
## tst_qwindow Test:
#####################################################################

add_qt_test(tst_qwindow
    SOURCES
        tst_qwindow.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::GuiPrivate
    PUBLIC_LIBRARIES
        Qt::Gui
)

## Scopes:
#####################################################################

extend_target(tst_qwindow CONDITION QT_FEATURE_dynamicgl AND WIN32 AND NOT WINRT
    PUBLIC_LIBRARIES
        user32
)

# Generated from qwidget.pro.

#####################################################################
## tst_qwidget Test:
#####################################################################

add_qt_test(tst_qwidget
    GUI
    SOURCES
        tst_qwidget.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::GuiPrivate
        Qt::TestPrivate
        Qt::WidgetsPrivate
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::Widgets
)

# Resources:
add_qt_resource(tst_qwidget "qwidget"
    PREFIX
        "/"
    FILES
        geometry-fullscreen.dat
        geometry-maximized.dat
        geometry.dat
)


#### Keys ignored in scope 1:.:.:qwidget.pro:<TRUE>:
# CONFIG = "testcase" "x11inc"
# testcase.timeout = "600"

## Scopes:
#####################################################################

extend_target(tst_qwidget CONDITION APPLE
    SOURCES
        tst_qwidget_mac_helpers.mm
    PUBLIC_LIBRARIES
        ${FWAppKit}
        ${FWSecurity}
)

extend_target(tst_qwidget CONDITION WIN32 AND NOT WINRT
    PUBLIC_LIBRARIES
        gdi32
        user32
)

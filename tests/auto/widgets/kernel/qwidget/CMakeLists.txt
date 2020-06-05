# Generated from qwidget.pro.

#####################################################################
## tst_qwidget Test:
#####################################################################

add_qt_test(tst_qwidget
    SOURCES
        tst_qwidget.cpp
    PUBLIC_LIBRARIES
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
        Qt::TestPrivate
        Qt::Widgets
        Qt::WidgetsPrivate
)

# Resources:
set(qwidget_resource_files
    "geometry-fullscreen.dat"
    "geometry-maximized.dat"
    "geometry.dat"
    "hellotr_la.qm"
)

add_qt_resource(tst_qwidget "qwidget"
    PREFIX
        "/"
    FILES
        ${qwidget_resource_files}
)


#### Keys ignored in scope 1:.:.:qwidget.pro:<TRUE>:
# testcase.timeout = "600"

## Scopes:
#####################################################################

extend_target(tst_qwidget CONDITION AIX
    COMPILE_OPTIONS
        -fpermissive
)

extend_target(tst_qwidget CONDITION APPLE
    SOURCES
        tst_qwidget_mac_helpers.mm
    PUBLIC_LIBRARIES
        ${FWAppKit}
        ${FWSecurity}
)

extend_target(tst_qwidget CONDITION WIN32
    PUBLIC_LIBRARIES
        gdi32
        user32
)

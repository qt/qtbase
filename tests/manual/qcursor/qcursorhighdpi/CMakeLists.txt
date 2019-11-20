# Generated from qcursorhighdpi.pro.

#####################################################################
## qcursorhighdpi Binary:
#####################################################################

add_qt_manual_test(qcursorhighdpi
    SOURCES
        main.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::GuiPrivate
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::Widgets
)

#### Keys ignored in scope 1:.:.:qcursorhighdpi.pro:<TRUE>:
# TEMPLATE = "app"

## Scopes:
#####################################################################

extend_target(qcursorhighdpi CONDITION WIN32
    PUBLIC_LIBRARIES
        User32
)

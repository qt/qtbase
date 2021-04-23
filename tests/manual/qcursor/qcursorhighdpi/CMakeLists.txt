# Generated from qcursorhighdpi.pro.

#####################################################################
## qcursorhighdpi Binary:
#####################################################################

qt_internal_add_manual_test(qcursorhighdpi
    SOURCES
        main.cpp
    PUBLIC_LIBRARIES
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
        Qt::Widgets
)

#### Keys ignored in scope 1:.:.:qcursorhighdpi.pro:<TRUE>:
# TEMPLATE = "app"

## Scopes:
#####################################################################

qt_internal_extend_target(qcursorhighdpi CONDITION WIN32
    PUBLIC_LIBRARIES
        user32
)

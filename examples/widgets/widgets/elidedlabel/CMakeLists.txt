# Generated from elidedlabel.pro.

#####################################################################
## elidedlabel Binary:
#####################################################################

add_qt_executable(elidedlabel
    GUI
    OUTPUT_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/widgets/elidedlabel"
    INSTALL_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/widgets/elidedlabel"
    SOURCES
        elidedlabel.cpp elidedlabel.h
        main.cpp
        testwidget.cpp testwidget.h
    LIBRARIES
        Qt::Gui
        Qt::Widgets
)

#### Keys ignored in scope 1:.:elidedlabel.pro:<NONE>:
# TEMPLATE = "app"

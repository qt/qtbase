# Generated from 7_selections.pro.

#####################################################################
## mv_selections Binary:
#####################################################################

add_qt_executable(mv_selections
    GUI
    OUTPUT_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/tutorials/modelview/7_selections"
    INSTALL_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/tutorials/modelview/7_selections"
    SOURCES
        main.cpp
        mainwindow.cpp mainwindow.h
    LIBRARIES
        Qt::Widgets
)

#### Keys ignored in scope 1:.:7_selections.pro:<NONE>:
# TEMPLATE = "app"

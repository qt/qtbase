# Generated from 5_edit.pro.

#####################################################################
## mv_edit Binary:
#####################################################################

add_qt_executable(mv_edit
    GUI
    OUTPUT_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/tutorials/modelview/5_edit"
    INSTALL_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/tutorials/modelview/5_edit"
    SOURCES
        main.cpp
        mainwindow.cpp mainwindow.h
        mymodel.cpp mymodel.h
    LIBRARIES
        Qt::Widgets
)

#### Keys ignored in scope 1:.:5_edit.pro:<NONE>:
# TEMPLATE = "app"

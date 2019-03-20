# Generated from undoframework.pro.

#####################################################################
## undoframework Binary:
#####################################################################

add_qt_executable(undoframework
    GUI
    OUTPUT_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/tools/undoframework"
    INSTALL_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/tools/undoframework"
    SOURCES
        commands.cpp commands.h
        diagramitem.cpp diagramitem.h
        diagramscene.cpp diagramscene.h
        main.cpp
        mainwindow.cpp mainwindow.h
    LIBRARIES
        Qt::Widgets
)

# Resources:
add_qt_resource(undoframework "undoframework" FILES
    images/cross.png)


# Generated from undo.pro.

#####################################################################
## undo Binary:
#####################################################################

add_qt_executable(undo
    GUI
    OUTPUT_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/tools/undo"
    INSTALL_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/tools/undo"
    SOURCES
        commands.cpp commands.h
        document.cpp document.h
        main.cpp
        mainwindow.cpp mainwindow.h mainwindow.ui
    LIBRARIES
        Qt::Widgets
)

# Resources:
add_qt_resource(undo "undo" PREFIX "/" FILES
    icons/background.png
    icons/blue.png
    icons/circle.png
    icons/exit.png
    icons/fileclose.png
    icons/filenew.png
    icons/fileopen.png
    icons/filesave.png
    icons/green.png
    icons/ok.png
    icons/rectangle.png
    icons/red.png
    icons/redo.png
    icons/remove.png
    icons/triangle.png
    icons/undo.png)


## Scopes:
#####################################################################

#### Keys ignored in scope 2:.:undo.pro:build_all AND NOT build_pass:
# CONFIG = "-build_all" "release"

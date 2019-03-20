# Generated from mainwindow.pro.

#####################################################################
## mainwindow Binary:
#####################################################################

add_qt_executable(mainwindow
    GUI
    OUTPUT_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/mainwindows/mainwindow"
    INSTALL_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/mainwindows/mainwindow"
    SOURCES
        colorswatch.cpp colorswatch.h
        main.cpp
        mainwindow.cpp mainwindow.h
        toolbar.cpp toolbar.h
    LIBRARIES
        Qt::Widgets
)

# Resources:
add_qt_resource(mainwindow "mainwindow" PREFIX "/res" FILES
    qt.png
    titlebarCenter.png
    titlebarLeft.png
    titlebarRight.png)


#### Keys ignored in scope 1:.:mainwindow.pro:<NONE>:
# TEMPLATE = "app"

## Scopes:
#####################################################################

#### Keys ignored in scope 2:.:mainwindow.pro:build_all AND NOT build_pass:
# CONFIG = "-build_all" "release"

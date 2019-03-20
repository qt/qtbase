# Generated from dockwidgets.pro.

#####################################################################
## dockwidgets Binary:
#####################################################################

add_qt_executable(dockwidgets
    GUI
    OUTPUT_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/mainwindows/dockwidgets"
    INSTALL_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/mainwindows/dockwidgets"
    SOURCES
        main.cpp
        mainwindow.cpp mainwindow.h
    LIBRARIES
        Qt::Widgets
)

# Resources:
add_qt_resource(dockwidgets "dockwidgets" FILES
    images/new.png
    images/print.png
    images/save.png
    images/undo.png)


## Scopes:
#####################################################################

extend_target(dockwidgets CONDITION TARGET Qt::PrintSupport
    LIBRARIES
        Qt::PrintSupport
)

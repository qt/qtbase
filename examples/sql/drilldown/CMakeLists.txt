# Generated from drilldown.pro.

#####################################################################
## drilldown Binary:
#####################################################################

add_qt_executable(drilldown
    GUI
    OUTPUT_DIRECTORY "${INSTALL_EXAMPLESDIR}/sql/drilldown"
    INSTALL_DIRECTORY "${INSTALL_EXAMPLESDIR}/sql/drilldown"
    SOURCES
        ../connection.h
        imageitem.cpp imageitem.h
        informationwindow.cpp informationwindow.h
        main.cpp
        view.cpp view.h
    LIBRARIES
        Qt::Sql
        Qt::Widgets
)

# Resources:
add_qt_resource(drilldown "drilldown" PREFIX "/" FILES
    images/qt-creator.png
    images/qt-logo.png
    images/qt-project.png
    images/qt-quick.png)


# Generated from application.pro.

#####################################################################
## application Binary:
#####################################################################

add_qt_executable(application
    GUI
    OUTPUT_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/mainwindows/application"
    INSTALL_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/mainwindows/application"
    SOURCES
        main.cpp
        mainwindow.cpp mainwindow.h
    LIBRARIES
        Qt::Widgets
)

# Resources:
add_qt_resource(application "application" FILES
    images/copy.png
    images/cut.png
    images/new.png
    images/open.png
    images/paste.png
    images/save.png)


# Generated from mdi.pro.

#####################################################################
## mdi Binary:
#####################################################################

add_qt_executable(mdi
    GUI
    OUTPUT_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/mainwindows/mdi"
    INSTALL_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/mainwindows/mdi"
    SOURCES
        main.cpp
        mainwindow.cpp mainwindow.h
        mdichild.cpp mdichild.h
    LIBRARIES
        Qt::Widgets
)

# Resources:
add_qt_resource(mdi "mdi" FILES
    images/copy.png
    images/cut.png
    images/new.png
    images/open.png
    images/paste.png
    images/save.png)


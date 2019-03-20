# Generated from sdi.pro.

#####################################################################
## sdi Binary:
#####################################################################

add_qt_executable(sdi
    GUI
    OUTPUT_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/mainwindows/sdi"
    INSTALL_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/mainwindows/sdi"
    SOURCES
        main.cpp
        mainwindow.cpp mainwindow.h
    LIBRARIES
        Qt::Widgets
)

# Resources:
add_qt_resource(sdi "sdi" FILES
    images/copy.png
    images/cut.png
    images/new.png
    images/open.png
    images/paste.png
    images/save.png)


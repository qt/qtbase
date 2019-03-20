# Generated from tablet.pro.

#####################################################################
## qttablet Binary:
#####################################################################

add_qt_executable(qttablet
    GUI
    OUTPUT_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/widgets/tablet"
    INSTALL_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/widgets/tablet"
    SOURCES
        main.cpp
        mainwindow.cpp mainwindow.h
        tabletapplication.cpp tabletapplication.h
        tabletcanvas.cpp tabletcanvas.h
    LIBRARIES
        Qt::Widgets
)

# Resources:
add_qt_resource(qttablet "images" FILES
    images/cursor-airbrush.png
    images/cursor-eraser.png
    images/cursor-felt-marker.png
    images/cursor-pencil.png)


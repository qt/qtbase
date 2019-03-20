# Generated from draggableicons.pro.

#####################################################################
## draggableicons Binary:
#####################################################################

add_qt_executable(draggableicons
    GUI
    OUTPUT_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/draganddrop/draggableicons"
    INSTALL_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/draganddrop/draggableicons"
    SOURCES
        dragwidget.cpp dragwidget.h
        main.cpp
    LIBRARIES
        Qt::Widgets
)

# Resources:
add_qt_resource(draggableicons "draggableicons" FILES
    images/boat.png
    images/car.png
    images/house.png)


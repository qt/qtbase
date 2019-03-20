# Generated from tooltips.pro.

#####################################################################
## tooltips Binary:
#####################################################################

add_qt_executable(tooltips
    GUI
    OUTPUT_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/widgets/tooltips"
    INSTALL_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/widgets/tooltips"
    SOURCES
        main.cpp
        shapeitem.cpp shapeitem.h
        sortingbox.cpp sortingbox.h
    LIBRARIES
        Qt::Widgets
)

# Resources:
add_qt_resource(tooltips "tooltips" FILES
    images/circle.png
    images/square.png
    images/triangle.png)


# Generated from imagecomposition.pro.

#####################################################################
## imagecomposition Binary:
#####################################################################

add_qt_executable(imagecomposition
    GUI
    OUTPUT_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/painting/imagecomposition"
    INSTALL_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/painting/imagecomposition"
    SOURCES
        imagecomposer.cpp imagecomposer.h
        main.cpp
    LIBRARIES
        Qt::Widgets
)

# Resources:
add_qt_resource(imagecomposition "imagecomposition" FILES
    images/butterfly.png
    images/checker.png)


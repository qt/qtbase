# Generated from pinchzoom.pro.

#####################################################################
## pinchzoom Binary:
#####################################################################

add_qt_executable(pinchzoom
    GUI
    OUTPUT_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/touch/pinchzoom"
    INSTALL_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/touch/pinchzoom"
    SOURCES
        graphicsview.cpp graphicsview.h
        main.cpp
        mouse.cpp mouse.h
    LIBRARIES
        Qt::Widgets
)

# Resources:
add_qt_resource(pinchzoom "mice" PREFIX "/" FILES
    images/cheese.jpg)


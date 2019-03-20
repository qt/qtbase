# Generated from collidingmice.pro.

#####################################################################
## collidingmice Binary:
#####################################################################

add_qt_executable(collidingmice
    GUI
    OUTPUT_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/graphicsview/collidingmice"
    INSTALL_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/graphicsview/collidingmice"
    SOURCES
        main.cpp
        mouse.cpp mouse.h
    LIBRARIES
        Qt::Widgets
)

# Resources:
add_qt_resource(collidingmice "mice" PREFIX "/" FILES
    images/cheese.jpg)


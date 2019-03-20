# Generated from stickman.pro.

#####################################################################
## stickman Binary:
#####################################################################

add_qt_executable(stickman
    GUI
    OUTPUT_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/animation/stickman"
    INSTALL_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/animation/stickman"
    SOURCES
        animation.cpp animation.h
        graphicsview.cpp graphicsview.h
        lifecycle.cpp lifecycle.h
        main.cpp
        node.cpp node.h
        rectbutton.cpp rectbutton.h
        stickman.cpp stickman.h
    LIBRARIES
        Qt::Widgets
)

# Resources:
add_qt_resource(stickman "stickman" FILES
    animations/chilling.bin
    animations/dancing.bin
    animations/dead.bin
    animations/jumping.bin)


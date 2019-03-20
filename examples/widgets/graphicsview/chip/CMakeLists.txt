# Generated from chip.pro.

#####################################################################
## chip Binary:
#####################################################################

add_qt_executable(chip
    GUI
    OUTPUT_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/graphicsview/chip"
    INSTALL_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/graphicsview/chip"
    SOURCES
        chip.cpp chip.h
        main.cpp
        mainwindow.cpp mainwindow.h
        view.cpp view.h
    LIBRARIES
        Qt::Widgets
)

# Resources:
add_qt_resource(chip "images" FILES
    fileprint.png
    qt4logo.png
    rotateleft.png
    rotateright.png
    zoomin.png
    zoomout.png)


## Scopes:
#####################################################################

extend_target(chip CONDITION TARGET Qt::PrintSupport
    LIBRARIES
        Qt::PrintSupport
)

extend_target(chip CONDITION TARGET Qt::OpenGL
    LIBRARIES
        Qt::OpenGL
)

#### Keys ignored in scope 4:.:chip.pro:build_all AND NOT build_pass:
# CONFIG = "-build_all" "release"

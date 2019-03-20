# Generated from boxes.pro.

find_package(WrapOpenGL)

#####################################################################
## boxes Binary:
#####################################################################

add_qt_executable(boxes
    GUI
    OUTPUT_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/graphicsview/boxes"
    INSTALL_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/graphicsview/boxes"
    SOURCES
        3rdparty/fbm.c 3rdparty/fbm.h
        glbuffers.cpp glbuffers.h
        glextensions.cpp glextensions.h
        gltrianglemesh.h
        main.cpp
        qtbox.cpp qtbox.h
        roundedbox.cpp roundedbox.h
        scene.cpp scene.h
        trackball.cpp trackball.h
    LIBRARIES
        Qt::OpenGL
        Qt::Widgets
        WrapOpenGL
)

# Resources:
add_qt_resource(boxes "boxes" PREFIX "/res/boxes" FILES
    basic.fsh
    basic.vsh
    cubemap_negx.jpg
    cubemap_negy.jpg
    cubemap_negz.jpg
    cubemap_posx.jpg
    cubemap_posy.jpg
    cubemap_posz.jpg
    dotted.fsh
    fresnel.fsh
    glass.fsh
    granite.fsh
    marble.fsh
    parameters.par
    qt-logo.jpg
    qt-logo.png
    reflection.fsh
    refraction.fsh
    smiley.png
    square.jpg
    wood.fsh)


## Scopes:
#####################################################################

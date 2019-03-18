# Generated from lightmaps.pro.

#####################################################################
## lightmaps Binary:
#####################################################################

add_qt_executable(lightmaps
    GUI
    OUTPUT_DIRECTORY "${INSTALL_EXAMPLESDIR}/embedded/lightmaps"
    INSTALL_DIRECTORY "${INSTALL_EXAMPLESDIR}/embedded/lightmaps"
    SOURCES
        lightmaps.cpp lightmaps.h
        main.cpp
        mapzoom.cpp mapzoom.h
        slippymap.cpp slippymap.h
    LIBRARIES
        Qt::Network
        Qt::Widgets
)

#### Keys ignored in scope 1:.:lightmaps.pro:<NONE>:
# TEMPLATE = "app"

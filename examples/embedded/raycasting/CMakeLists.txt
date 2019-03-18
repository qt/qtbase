# Generated from raycasting.pro.

#####################################################################
## raycasting Binary:
#####################################################################

add_qt_executable(raycasting
    GUI
    OUTPUT_DIRECTORY "${INSTALL_EXAMPLESDIR}/embedded/raycasting"
    INSTALL_DIRECTORY "${INSTALL_EXAMPLESDIR}/embedded/raycasting"
    SOURCES
        raycasting.cpp
    LIBRARIES
        Qt::Widgets
)

# Resources:
add_qt_resource(raycasting "raycasting" PREFIX "/" FILES
    textures.png)


#### Keys ignored in scope 1:.:raycasting.pro:<NONE>:
# TEMPLATE = "app"

# Generated from animatedtiles.pro.

#####################################################################
## animatedtiles Binary:
#####################################################################

add_qt_executable(animatedtiles
    GUI
    OUTPUT_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/animation/animatedtiles"
    INSTALL_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/animation/animatedtiles"
    SOURCES
        main.cpp
    LIBRARIES
        Qt::Widgets
)

# Resources:
add_qt_resource(animatedtiles "animatedtiles" FILES
    images/Time-For-Lunch-2.jpg
    images/centered.png
    images/ellipse.png
    images/figure8.png
    images/kinetic.png
    images/random.png
    images/tile.png)


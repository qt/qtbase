# Generated from weatheranchorlayout.pro.

#####################################################################
## weatheranchorlayout Binary:
#####################################################################

add_qt_executable(weatheranchorlayout
    GUI
    OUTPUT_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/graphicsview/weatheranchorlayout"
    INSTALL_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/graphicsview/weatheranchorlayout"
    SOURCES
        main.cpp
    LIBRARIES
        Qt::Widgets
)

# Resources:
add_qt_resource(weatheranchorlayout "weatheranchorlayout" FILES
    images/5days.jpg
    images/details.jpg
    images/place.jpg
    images/tabbar.jpg
    images/title.jpg
    images/weather-few-clouds.png)


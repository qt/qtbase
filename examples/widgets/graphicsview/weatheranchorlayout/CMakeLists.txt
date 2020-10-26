# Generated from weatheranchorlayout.pro.

cmake_minimum_required(VERSION 3.14)
project(weatheranchorlayout LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
  set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/widgets/graphicsview/weatheranchorlayout")

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS Gui)
find_package(Qt6 COMPONENTS Widgets)

qt_add_executable(weatheranchorlayout
    main.cpp
)
set_target_properties(weatheranchorlayout PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)
target_link_libraries(weatheranchorlayout PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Widgets
)


# Resources:
set(weatheranchorlayout_resource_files
    "images/5days.jpg"
    "images/details.jpg"
    "images/place.jpg"
    "images/tabbar.jpg"
    "images/title.jpg"
    "images/weather-few-clouds.png"
)

qt6_add_resources(weatheranchorlayout "weatheranchorlayout"
    PREFIX
        "/"
    FILES
        ${weatheranchorlayout_resource_files}
)

install(TARGETS weatheranchorlayout
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)

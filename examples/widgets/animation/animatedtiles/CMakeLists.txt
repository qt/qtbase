# Generated from animatedtiles.pro.

cmake_minimum_required(VERSION 3.14)
project(animatedtiles LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
  set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/widgets/animation/animatedtiles")

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS Gui)
find_package(Qt6 COMPONENTS Widgets)

add_qt_gui_executable(animatedtiles
    main.cpp
)
target_link_libraries(animatedtiles PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Widgets
)


# Resources:
set(animatedtiles_resource_files
    "images/Time-For-Lunch-2.jpg"
    "images/centered.png"
    "images/ellipse.png"
    "images/figure8.png"
    "images/kinetic.png"
    "images/random.png"
    "images/tile.png"
)

qt6_add_resources(animatedtiles "animatedtiles"
    PREFIX
        "/"
    FILES
        ${animatedtiles_resource_files}
)

install(TARGETS animatedtiles
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)

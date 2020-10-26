# Generated from draggableicons.pro.

cmake_minimum_required(VERSION 3.14)
project(draggableicons LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
  set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/widgets/draganddrop/draggableicons")

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS Gui)
find_package(Qt6 COMPONENTS Widgets)

qt_add_executable(draggableicons
    dragwidget.cpp dragwidget.h
    main.cpp
)
set_target_properties(draggableicons PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)
target_link_libraries(draggableicons PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Widgets
)


# Resources:
set(draggableicons_resource_files
    "images/boat.png"
    "images/car.png"
    "images/house.png"
)

qt6_add_resources(draggableicons "draggableicons"
    PREFIX
        "/"
    FILES
        ${draggableicons_resource_files}
)

install(TARGETS draggableicons
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)

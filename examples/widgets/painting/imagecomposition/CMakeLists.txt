# Generated from imagecomposition.pro.

cmake_minimum_required(VERSION 3.14)
project(imagecomposition LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
  set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/widgets/painting/imagecomposition")

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS Gui)
find_package(Qt6 COMPONENTS Widgets)

qt_add_executable(imagecomposition
    imagecomposer.cpp imagecomposer.h
    main.cpp
)
set_target_properties(imagecomposition PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)
target_link_libraries(imagecomposition PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Widgets
)


# Resources:
set(imagecomposition_resource_files
    "images/butterfly.png"
    "images/checker.png"
)

qt6_add_resources(imagecomposition "imagecomposition"
    PREFIX
        "/"
    FILES
        ${imagecomposition_resource_files}
)

install(TARGETS imagecomposition
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)

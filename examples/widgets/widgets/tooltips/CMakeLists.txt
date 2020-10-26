# Generated from tooltips.pro.

cmake_minimum_required(VERSION 3.14)
project(tooltips LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
  set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/widgets/widgets/tooltips")

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS Gui)
find_package(Qt6 COMPONENTS Widgets)

qt_add_executable(tooltips
    main.cpp
    shapeitem.cpp shapeitem.h
    sortingbox.cpp sortingbox.h
)
set_target_properties(tooltips PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)
target_link_libraries(tooltips PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Widgets
)


# Resources:
set(tooltips_resource_files
    "images/circle.png"
    "images/square.png"
    "images/triangle.png"
)

qt6_add_resources(tooltips "tooltips"
    PREFIX
        "/"
    FILES
        ${tooltips_resource_files}
)

install(TARGETS tooltips
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)

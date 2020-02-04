# Generated from classwizard.pro.

cmake_minimum_required(VERSION 3.14)
project(classwizard LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(INSTALL_EXAMPLEDIR "examples/widgets/dialogs/classwizard")

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS Gui)
find_package(Qt6 COMPONENTS Widgets)

add_qt_gui_executable(classwizard
    classwizard.cpp classwizard.h
    main.cpp
)
target_link_libraries(classwizard PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Widgets
)


# Resources:
set(classwizard_resource_files
    "images/background.png"
    "images/banner.png"
    "images/logo1.png"
    "images/logo2.png"
    "images/logo3.png"
    "images/watermark1.png"
    "images/watermark2.png"
)

qt6_add_resources(classwizard "classwizard"
    PREFIX
        "/"
    FILES
        ${classwizard_resource_files}
)

install(TARGETS classwizard
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)

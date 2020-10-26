# Generated from classwizard.pro.

cmake_minimum_required(VERSION 3.14)
project(classwizard LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
  set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/widgets/dialogs/classwizard")

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS Gui)
find_package(Qt6 COMPONENTS Widgets)

qt_add_executable(classwizard
    classwizard.cpp classwizard.h
    main.cpp
)
set_target_properties(classwizard PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
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

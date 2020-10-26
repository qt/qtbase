# Generated from systray.pro.

cmake_minimum_required(VERSION 3.14)
project(systray LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
  set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/widgets/desktop/systray")

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS Gui)
find_package(Qt6 COMPONENTS Widgets)

qt_add_executable(systray
    main.cpp
    window.cpp window.h
)
set_target_properties(systray PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)
target_link_libraries(systray PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Widgets
)


# Resources:
set(systray_resource_files
    "images/bad.png"
    "images/heart.png"
    "images/trash.png"
)

qt6_add_resources(systray "systray"
    PREFIX
        "/"
    FILES
        ${systray_resource_files}
)

install(TARGETS systray
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)

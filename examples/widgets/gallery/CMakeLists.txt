# Generated from gallery.pro.

cmake_minimum_required(VERSION 3.14)
project(gallery LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(INSTALL_EXAMPLEDIR "examples/widgets/gallery")

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS Gui)
find_package(Qt6 COMPONENTS Widgets)

add_qt_gui_executable(gallery
    main.cpp
    widgetgallery.cpp widgetgallery.h
)
target_link_libraries(gallery PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Widgets
)

install(TARGETS gallery
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)

# Generated from transformations.pro.

cmake_minimum_required(VERSION 3.14)
project(transformations LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(INSTALL_EXAMPLEDIR "examples/widgets/painting/transformations")

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS Gui)
find_package(Qt6 COMPONENTS Widgets)

add_qt_gui_executable(transformations
    main.cpp
    renderarea.cpp renderarea.h
    window.cpp window.h
)
target_link_libraries(transformations PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Widgets
)

install(TARGETS transformations
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)

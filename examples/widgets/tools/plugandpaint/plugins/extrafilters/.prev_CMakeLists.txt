# Generated from extrafilters.pro.

cmake_minimum_required(VERSION 3.14)
project(pnp_extrafilters LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(INSTALL_EXAMPLEDIR "examples/widgets/tools/plugandpaint/plugins")

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS Gui)
find_package(Qt6 COMPONENTS Widgets)

qt_add_plugin(pnp_extrafilters)
target_sources(pnp_extrafilters PRIVATE
    extrafiltersplugin.cpp extrafiltersplugin.h
)
target_include_directories(pnp_extrafilters PUBLIC
    ../../app
)

target_link_libraries(pnp_extrafilters PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Widgets
)

install(TARGETS pnp_extrafilters
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)

# Generated from lightmaps.pro.

cmake_minimum_required(VERSION 3.14)
project(lightmaps LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(INSTALL_EXAMPLEDIR "examples")

find_package(Qt6 COMPONENTS Network)
find_package(Qt6 COMPONENTS Widgets)

add_qt_gui_executable(lightmaps
    lightmaps.cpp lightmaps.h
    main.cpp
    mapzoom.cpp mapzoom.h
    slippymap.cpp slippymap.h
)
target_link_libraries(lightmaps PUBLIC
    Qt::Network
    Qt::Widgets
)
install(TARGETS lightmaps
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)

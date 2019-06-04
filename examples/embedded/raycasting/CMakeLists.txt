# Generated from raycasting.pro.

cmake_minimum_required(VERSION 3.14)
project(raycasting LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(INSTALL_EXAMPLEDIR "examples")

find_package(Qt5 COMPONENTS Widgets)

add_executable(raycasting WIN32 MACOSX_BUNDLE
    raycasting.cpp raycasting.qrc
)
target_link_libraries(raycasting PUBLIC
    Qt::Widgets
)

install(TARGETS raycasting
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
)

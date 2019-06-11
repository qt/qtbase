# Generated from blurpicker.pro.

cmake_minimum_required(VERSION 3.14)
project(blurpicker LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(INSTALL_EXAMPLEDIR "examples")

find_package(Qt5 COMPONENTS Widgets)

add_qt_gui_executable(blurpicker WIN32 MACOSX_BUNDLE
    blureffect.cpp blureffect.h
    blurpicker.cpp blurpicker.h blurpicker.qrc
    main.cpp
)
target_link_libraries(blurpicker PUBLIC
    Qt::Widgets
)

install(TARGETS blurpicker
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
)

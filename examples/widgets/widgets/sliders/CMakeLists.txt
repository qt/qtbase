# Generated from sliders.pro.

cmake_minimum_required(VERSION 3.14)
project(sliders LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(INSTALL_EXAMPLEDIR "examples")

find_package(Qt5 COMPONENTS Widgets)

add_executable(sliders WIN32 MACOSX_BUNDLE
    main.cpp
    slidersgroup.cpp slidersgroup.h
    window.cpp window.h
)
target_link_libraries(sliders PUBLIC
    Qt::Widgets
)

install(TARGETS sliders
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
)

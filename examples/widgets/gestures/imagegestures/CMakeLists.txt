# Generated from imagegestures.pro.

cmake_minimum_required(VERSION 3.14)
project(imagegestures LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(INSTALL_EXAMPLEDIR "examples")

find_package(Qt5 COMPONENTS Widgets)

add_executable(imagegestures WIN32 MACOSX_BUNDLE
    imagewidget.cpp imagewidget.h
    main.cpp
    mainwidget.cpp mainwidget.h
)
target_link_libraries(imagegestures PUBLIC
    Qt::Widgets
)

install(TARGETS imagegestures
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
)

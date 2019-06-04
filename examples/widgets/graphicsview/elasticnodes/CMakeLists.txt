# Generated from elasticnodes.pro.

cmake_minimum_required(VERSION 3.14)
project(elasticnodes LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(INSTALL_EXAMPLEDIR "examples")

find_package(Qt5 COMPONENTS Widgets)

add_executable(elasticnodes WIN32 MACOSX_BUNDLE
    edge.cpp edge.h
    graphwidget.cpp graphwidget.h
    main.cpp
    node.cpp node.h
)
target_link_libraries(elasticnodes PUBLIC
    Qt::Widgets
)

install(TARGETS elasticnodes
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
)

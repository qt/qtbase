# Generated from simpledommodel.pro.

cmake_minimum_required(VERSION 3.14)
project(simpledommodel LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(INSTALL_EXAMPLEDIR "examples")

find_package(Qt5 COMPONENTS Xml)
find_package(Qt5 COMPONENTS Widgets)

add_executable(simpledommodel WIN32 MACOSX_BUNDLE
    domitem.cpp domitem.h
    dommodel.cpp dommodel.h
    main.cpp
    mainwindow.cpp mainwindow.h
)
target_link_libraries(simpledommodel PUBLIC
    Qt::Widgets
    Qt::Xml
)

install(TARGETS simpledommodel
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
)

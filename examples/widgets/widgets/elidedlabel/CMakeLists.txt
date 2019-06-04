# Generated from elidedlabel.pro.

cmake_minimum_required(VERSION 3.14)
project(elidedlabel LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(INSTALL_EXAMPLEDIR "examples")

find_package(Qt5 COMPONENTS Core)
find_package(Qt5 COMPONENTS Gui)
find_package(Qt5 COMPONENTS Widgets)

add_executable(elidedlabel WIN32 MACOSX_BUNDLE
    elidedlabel.cpp elidedlabel.h
    main.cpp
    testwidget.cpp testwidget.h
)
target_link_libraries(elidedlabel PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Widgets
)

install(TARGETS elidedlabel
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
)

# Generated from storageview.pro.

cmake_minimum_required(VERSION 3.14)
project(storageview LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(INSTALL_EXAMPLEDIR "examples")

find_package(Qt5 COMPONENTS Core)
find_package(Qt5 COMPONENTS Gui)
find_package(Qt5 COMPONENTS Widgets)

add_executable(storageview WIN32 MACOSX_BUNDLE
    main.cpp
    storagemodel.cpp storagemodel.h
)
target_link_libraries(storageview PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Widgets
)

install(TARGETS storageview
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
)

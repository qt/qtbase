# Generated from sharedmemory.pro.

cmake_minimum_required(VERSION 3.14)
project(sharedmemory LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(INSTALL_EXAMPLEDIR "examples")

find_package(Qt5 COMPONENTS Widgets)

add_executable(sharedmemory WIN32 MACOSX_BUNDLE
    dialog.cpp dialog.h dialog.ui
    main.cpp
)
target_link_libraries(sharedmemory PUBLIC
    Qt::Widgets
)
install(TARGETS sharedmemory
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
)

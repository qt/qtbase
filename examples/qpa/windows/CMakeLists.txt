# Generated from windows.pro.

cmake_minimum_required(VERSION 3.14)
project(windows LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(INSTALL_EXAMPLEDIR "examples/qpa/windows")

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS Gui)

add_executable(windows
    main.cpp
    window.cpp window.h
)
target_link_libraries(windows PRIVATE
    Qt::CorePrivate
    Qt::GuiPrivate
)

target_link_libraries(windows PUBLIC
    Qt::Core
    Qt::Gui
)

install(TARGETS windows
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)

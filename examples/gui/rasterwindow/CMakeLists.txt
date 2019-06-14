# Generated from rasterwindow.pro.

cmake_minimum_required(VERSION 3.14)
project(rasterwindow LANGUAGES CXX)

find_package(Qt6 COMPONENTS Widgets) # special case: add

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(INSTALL_EXAMPLEDIR "examples")

add_qt_gui_executable(rasterwindow
    main.cpp
    rasterwindow.cpp rasterwindow.h
)
target_include_directories(rasterwindow PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
# special case begin
target_link_libraries(rasterwindow PUBLIC
    Qt::Gui
)
# special case end
install(TARGETS rasterwindow
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)

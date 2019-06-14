# Generated from analogclock.pro.

cmake_minimum_required(VERSION 3.14)
project(analogclock LANGUAGES CXX)

find_package(Qt6 COMPONENTS Widgets) # special case: add

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(INSTALL_EXAMPLEDIR "examples")

add_qt_gui_executable(gui_analogclock # special case: renamed target
    ../rasterwindow/rasterwindow.cpp ../rasterwindow/rasterwindow.h
    main.cpp
)
target_include_directories(gui_analogclock PUBLIC
    ../rasterwindow
)
# special case begin
target_link_libraries(gui_analogclock PUBLIC
    Qt::Gui
)
# special case end

install(TARGETS gui_analogclock
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)

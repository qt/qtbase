# Generated from analogclock.pro.

cmake_minimum_required(VERSION 3.14)
project(analogclock LANGUAGES CXX)


set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
  set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/gui/gui_analogclock") # special case

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS Gui)
find_package(Qt6 COMPONENTS Widgets) # special case: add

qt_add_executable(gui_analogclock # special case: renamed target
    ../rasterwindow/rasterwindow.cpp ../rasterwindow/rasterwindow.h
    main.cpp
)
set_target_properties(gui_analogclock PROPERTIES # special case
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)
target_include_directories(gui_analogclock PUBLIC # special case
    ../rasterwindow
)
# special case begin
target_link_libraries(gui_analogclock PUBLIC # special case
    Qt::Gui
)
# special case end

target_link_libraries(gui_analogclock PUBLIC # special case
    Qt::Core
    Qt::Gui
)

install(TARGETS gui_analogclock # special case
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)

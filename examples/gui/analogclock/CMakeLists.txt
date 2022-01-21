cmake_minimum_required(VERSION 3.16)
project(analogclock LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/gui/gui_analogclock")

find_package(Qt6 REQUIRED COMPONENTS Core Gui)

qt_add_executable(gui_analogclock
    ../rasterwindow/rasterwindow.cpp ../rasterwindow/rasterwindow.h
    main.cpp
)

set_target_properties(gui_analogclock PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)

target_include_directories(gui_analogclock PUBLIC
    ../rasterwindow
)

target_link_libraries(gui_analogclock PUBLIC
    Qt::Core
    Qt::Gui
)

install(TARGETS gui_analogclock
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)

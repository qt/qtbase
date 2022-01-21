cmake_minimum_required(VERSION 3.16)
project(windowcontainer LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/widgets/windowcontainer")

find_package(Qt6 REQUIRED COMPONENTS Core Gui OpenGL Widgets)

qt_add_executable(windowcontainer
    ../../opengl/openglwindow/openglwindow.cpp ../../opengl/openglwindow/openglwindow.h
    windowcontainer.cpp
)

set_target_properties(windowcontainer PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)

target_include_directories(windowcontainer PUBLIC
    ../../opengl/openglwindow
)

target_link_libraries(windowcontainer PUBLIC
    Qt::Core
    Qt::Gui
    Qt::OpenGL
    Qt::Widgets
)

install(TARGETS windowcontainer
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)

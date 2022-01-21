cmake_minimum_required(VERSION 3.16)
project(textures LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/opengl/textures")

find_package(Qt6 REQUIRED COMPONENTS Core Gui OpenGL OpenGLWidgets Widgets)

qt_add_executable(textures
    glwidget.cpp glwidget.h
    main.cpp
    window.cpp window.h
)

set_target_properties(textures PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)

target_link_libraries(textures PUBLIC
    Qt::Core
    Qt::Gui
    Qt::OpenGL
    Qt::OpenGLWidgets
    Qt::Widgets
)

# Resources:
set(textures_resource_files
    "images/side1.png"
    "images/side2.png"
    "images/side3.png"
    "images/side4.png"
    "images/side5.png"
    "images/side6.png"
)

qt6_add_resources(textures "textures"
    PREFIX
        "/"
    FILES
        ${textures_resource_files}
)

install(TARGETS textures
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)

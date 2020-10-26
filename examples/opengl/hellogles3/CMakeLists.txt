# Generated from hellogles3.pro.

cmake_minimum_required(VERSION 3.14)
project(hellogles3 LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
  set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/opengl/hellogles3")

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS Gui)
find_package(Qt6 COMPONENTS OpenGL)

qt_add_executable(hellogles3
    ../hellogl2/logo.cpp ../hellogl2/logo.h
    glwindow.cpp glwindow.h
    main.cpp
)
set_target_properties(hellogles3 PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)
target_link_libraries(hellogles3 PUBLIC
    Qt::Core
    Qt::Gui
    Qt::OpenGL
)


# Resources:
set(hellogles3_resource_files
    "qtlogo.png"
)

qt6_add_resources(hellogles3 "hellogles3"
    PREFIX
        "/"
    FILES
        ${hellogles3_resource_files}
)

install(TARGETS hellogles3
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)

# Generated from boxes.pro.

cmake_minimum_required(VERSION 3.14)
project(boxes LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(INSTALL_EXAMPLEDIR "examples")

find_package(Qt5 COMPONENTS OpenGL)
find_package(Qt5 COMPONENTS Widgets)


add_executable(boxes WIN32 MACOSX_BUNDLE
    3rdparty/fbm.c 3rdparty/fbm.h
    boxes.qrc
    glbuffers.cpp glbuffers.h
    glextensions.cpp glextensions.h
    gltrianglemesh.h
    main.cpp
    qtbox.cpp qtbox.h
    roundedbox.cpp roundedbox.h
    scene.cpp scene.h
    trackball.cpp trackball.h
)
target_link_libraries(boxes PUBLIC
    Qt::OpenGL
    Qt::Widgets
)

install(TARGETS boxes
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
)

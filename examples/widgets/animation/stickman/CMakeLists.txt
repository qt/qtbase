# Generated from stickman.pro.

cmake_minimum_required(VERSION 3.14)
project(stickman LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
  set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/widgets/animation/stickman")

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS Gui)
find_package(Qt6 COMPONENTS Widgets)

add_qt_gui_executable(stickman
    animation.cpp animation.h
    graphicsview.cpp graphicsview.h
    lifecycle.cpp lifecycle.h
    main.cpp
    node.cpp node.h
    rectbutton.cpp rectbutton.h
    stickman.cpp stickman.h
)
target_link_libraries(stickman PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Widgets
)


# Resources:
set(stickman_resource_files
    "animations/chilling.bin"
    "animations/dancing.bin"
    "animations/dead.bin"
    "animations/jumping.bin"
)

qt6_add_resources(stickman "stickman"
    PREFIX
        "/"
    FILES
        ${stickman_resource_files}
)

install(TARGETS stickman
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)

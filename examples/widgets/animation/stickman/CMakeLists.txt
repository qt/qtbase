# Generated from stickman.pro.

cmake_minimum_required(VERSION 3.14)
project(stickman LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(INSTALL_EXAMPLEDIR "examples")

find_package(Qt6 COMPONENTS Widgets)

add_qt_gui_executable(stickman
    animation.cpp animation.h
    graphicsview.cpp graphicsview.h
    lifecycle.cpp lifecycle.h
    main.cpp
    node.cpp node.h
    rectbutton.cpp rectbutton.h
    stickman.cpp stickman.h stickman.qrc
)
target_link_libraries(stickman PUBLIC
    Qt::Widgets
)

install(TARGETS stickman
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)

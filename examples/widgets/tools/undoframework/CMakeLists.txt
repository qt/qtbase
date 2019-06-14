# Generated from undoframework.pro.

cmake_minimum_required(VERSION 3.14)
project(undoframework LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(INSTALL_EXAMPLEDIR "examples")

find_package(Qt6 COMPONENTS Widgets)

add_qt_gui_executable(undoframework
    commands.cpp commands.h
    diagramitem.cpp diagramitem.h
    diagramscene.cpp diagramscene.h
    main.cpp
    mainwindow.cpp mainwindow.h
    undoframework.qrc
)
target_link_libraries(undoframework PUBLIC
    Qt::Widgets
)

install(TARGETS undoframework
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)

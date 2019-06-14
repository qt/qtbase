# Generated from diagramscene.pro.

cmake_minimum_required(VERSION 3.14)
project(diagramscene LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(INSTALL_EXAMPLEDIR "examples")

find_package(Qt6 COMPONENTS Widgets)

add_qt_gui_executable(diagramscene
    arrow.cpp arrow.h
    diagramitem.cpp diagramitem.h
    diagramscene.cpp diagramscene.h diagramscene.qrc
    diagramtextitem.cpp diagramtextitem.h
    main.cpp
    mainwindow.cpp mainwindow.h
)
target_link_libraries(diagramscene PUBLIC
    Qt::Widgets
)

install(TARGETS diagramscene
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)

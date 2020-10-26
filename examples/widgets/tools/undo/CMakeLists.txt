# Generated from undo.pro.

cmake_minimum_required(VERSION 3.14)
project(undo LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
  set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/widgets/tools/undo")

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS Gui)
find_package(Qt6 COMPONENTS Widgets)

qt_add_executable(undo
    commands.cpp commands.h
    document.cpp document.h
    main.cpp
    mainwindow.cpp mainwindow.h mainwindow.ui
)
set_target_properties(undo PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)
target_link_libraries(undo PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Widgets
)


# Resources:
set(undo_resource_files
    "icons/background.png"
    "icons/blue.png"
    "icons/circle.png"
    "icons/exit.png"
    "icons/fileclose.png"
    "icons/filenew.png"
    "icons/fileopen.png"
    "icons/filesave.png"
    "icons/green.png"
    "icons/ok.png"
    "icons/rectangle.png"
    "icons/red.png"
    "icons/redo.png"
    "icons/remove.png"
    "icons/triangle.png"
    "icons/undo.png"
)

qt6_add_resources(undo "undo"
    PREFIX
        "/"
    FILES
        ${undo_resource_files}
)

install(TARGETS undo
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)

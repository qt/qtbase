# Generated from dombookmarks.pro.

cmake_minimum_required(VERSION 3.14)
project(dombookmarks LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
  set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/xml/dombookmarks")

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS Gui)
find_package(Qt6 COMPONENTS Xml)
find_package(Qt6 COMPONENTS Widgets)

qt_add_executable(dombookmarks
    main.cpp
    mainwindow.cpp mainwindow.h
    xbeltree.cpp xbeltree.h
)
set_target_properties(dombookmarks PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)
target_link_libraries(dombookmarks PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Widgets
    Qt::Xml
)

install(TARGETS dombookmarks
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)

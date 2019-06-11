# Generated from wiggly.pro.

cmake_minimum_required(VERSION 3.14)
project(wiggly LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(INSTALL_EXAMPLEDIR "examples")

find_package(Qt5 COMPONENTS Widgets)

add_qt_gui_executable(wiggly WIN32 MACOSX_BUNDLE
    dialog.cpp dialog.h
    main.cpp
    wigglywidget.cpp wigglywidget.h
)
target_link_libraries(wiggly PUBLIC
    Qt::Widgets
)

install(TARGETS wiggly
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
)

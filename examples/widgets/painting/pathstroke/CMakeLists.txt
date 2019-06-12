# Generated from pathstroke.pro.

cmake_minimum_required(VERSION 3.14)
project(pathstroke LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(INSTALL_EXAMPLEDIR "examples")

find_package(Qt5 COMPONENTS Widgets)

add_qt_gui_executable(pathstroke
    ../shared/arthurstyle.cpp ../shared/arthurstyle.h
    ../shared/arthurwidgets.cpp ../shared/arthurwidgets.h
    ../shared/hoverpoints.cpp ../shared/hoverpoints.h
    ../shared/shared.qrc
    main.cpp
    pathstroke.cpp pathstroke.h pathstroke.qrc
)
target_include_directories(pathstroke PUBLIC
    ../shared
)
target_link_libraries(pathstroke PUBLIC
    Qt::Widgets
)

install(TARGETS pathstroke
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)

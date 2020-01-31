# Generated from basicdrawing.pro.

cmake_minimum_required(VERSION 3.14)
project(basicdrawing LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(INSTALL_EXAMPLEDIR "examples")

find_package(Qt6 COMPONENTS Widgets)

add_qt_gui_executable(basicdrawing
    basicdrawing.qrc
    main.cpp
    renderarea.cpp renderarea.h
    window.cpp window.h
)
target_link_libraries(basicdrawing PUBLIC
    Qt::Widgets
)

install(TARGETS basicdrawing
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)

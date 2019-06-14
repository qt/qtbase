# Generated from styles.pro.

cmake_minimum_required(VERSION 3.14)
project(styles LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(INSTALL_EXAMPLEDIR "examples")

find_package(Qt6 COMPONENTS Widgets)

add_qt_gui_executable(styles
    main.cpp
    norwegianwoodstyle.cpp norwegianwoodstyle.h
    styles.qrc
    widgetgallery.cpp widgetgallery.h
)
target_link_libraries(styles PUBLIC
    Qt::Widgets
)

install(TARGETS styles
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)

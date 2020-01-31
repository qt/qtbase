# Generated from queuedcustomtype.pro.

cmake_minimum_required(VERSION 3.14)
project(queuedcustomtype LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(INSTALL_EXAMPLEDIR "examples")

find_package(Qt6 COMPONENTS Widgets)

add_qt_gui_executable(queuedcustomtype
    block.cpp block.h
    main.cpp
    renderthread.cpp renderthread.h
    window.cpp window.h
)
target_link_libraries(queuedcustomtype PUBLIC
    Qt::Widgets
)
install(TARGETS queuedcustomtype
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)

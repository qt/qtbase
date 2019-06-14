# Generated from stardelegate.pro.

cmake_minimum_required(VERSION 3.14)
project(stardelegate LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(INSTALL_EXAMPLEDIR "examples")

find_package(Qt6 COMPONENTS Widgets)

add_qt_gui_executable(stardelegate
    main.cpp
    stardelegate.cpp stardelegate.h
    stareditor.cpp stareditor.h
    starrating.cpp starrating.h
)
target_link_libraries(stardelegate PUBLIC
    Qt::Widgets
)

install(TARGETS stardelegate
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)

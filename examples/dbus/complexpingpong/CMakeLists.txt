# Generated from complexpong.pro.

cmake_minimum_required(VERSION 3.14)
project(complexpong LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(INSTALL_EXAMPLEDIR "examples")

find_package(Qt6 COMPONENTS DBus)

add_qt_gui_executable(complexpong
    complexpong.cpp complexpong.h
)
target_link_libraries(complexpong PUBLIC
    # Remove: gui
    Qt::DBus
)
install(TARGETS complexpong
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)

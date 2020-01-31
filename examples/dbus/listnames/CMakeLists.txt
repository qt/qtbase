# Generated from listnames.pro.

cmake_minimum_required(VERSION 3.14)
project(listnames LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(INSTALL_EXAMPLEDIR "examples")

find_package(Qt6 COMPONENTS DBus)

add_qt_gui_executable(listnames
    listnames.cpp
)
target_link_libraries(listnames PUBLIC
    # Remove: gui
    Qt::DBus
)
install(TARGETS listnames
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)

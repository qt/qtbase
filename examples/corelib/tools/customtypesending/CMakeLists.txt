# Generated from customtypesending.pro.

cmake_minimum_required(VERSION 3.14)
project(customtypesending LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(INSTALL_EXAMPLEDIR "examples")

find_package(Qt6 COMPONENTS Widgets)

add_qt_gui_executable(customtypesending
    main.cpp
    message.cpp message.h
    window.cpp window.h
)
target_link_libraries(customtypesending PUBLIC
    Qt::Widgets
)
install(TARGETS customtypesending
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)

# Generated from localfortuneserver.pro.

cmake_minimum_required(VERSION 3.14)
project(localfortuneserver LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(INSTALL_EXAMPLEDIR "examples")

find_package(Qt6 COMPONENTS Network)
find_package(Qt6 COMPONENTS Widgets)

add_qt_gui_executable(localfortuneserver
    main.cpp
    server.cpp server.h
)
target_link_libraries(localfortuneserver PUBLIC
    Qt::Network
    Qt::Widgets
)
install(TARGETS localfortuneserver
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)

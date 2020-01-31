# Generated from htmlinfo.pro.

cmake_minimum_required(VERSION 3.14)
project(htmlinfo LANGUAGES CXX)

find_package(Qt6 COMPONENTS Core) # special case: add

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(INSTALL_EXAMPLEDIR "examples")

add_executable(htmlinfo
    main.cpp
)
target_link_libraries(htmlinfo PUBLIC
    Qt::Core
)
install(TARGETS htmlinfo
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)

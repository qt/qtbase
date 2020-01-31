# Generated from map.pro.

cmake_minimum_required(VERSION 3.14)
project(mapdemo LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(INSTALL_EXAMPLEDIR "examples")

find_package(Qt6 COMPONENTS Concurrent)
find_package(Qt6 COMPONENTS Gui) # special case

add_executable(mapdemo
    main.cpp
)
target_link_libraries(mapdemo PUBLIC
    Qt::Concurrent
    Qt::Gui # special case
)
install(TARGETS mapdemo
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)

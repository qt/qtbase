# Generated from wordcount.pro.

cmake_minimum_required(VERSION 3.14)
project(wordcount LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(INSTALL_EXAMPLEDIR "examples")

find_package(Qt6 COMPONENTS Concurrent)
find_package(Qt6 COMPONENTS Widgets)

add_executable(wordcount
    main.cpp
)
target_link_libraries(wordcount PUBLIC
    Qt::Concurrent
    Qt::Widgets
)

install(TARGETS wordcount
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)

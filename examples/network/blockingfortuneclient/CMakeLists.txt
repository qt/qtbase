# Generated from blockingfortuneclient.pro.

cmake_minimum_required(VERSION 3.14)
project(blockingfortuneclient LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(INSTALL_EXAMPLEDIR "examples")

find_package(Qt5 COMPONENTS Network)
find_package(Qt5 COMPONENTS Widgets)

add_executable(blockingfortuneclient WIN32 MACOSX_BUNDLE
    blockingclient.cpp blockingclient.h
    fortunethread.cpp fortunethread.h
    main.cpp
)
target_link_libraries(blockingfortuneclient PUBLIC
    Qt::Network
    Qt::Widgets
)

install(TARGETS blockingfortuneclient
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
)

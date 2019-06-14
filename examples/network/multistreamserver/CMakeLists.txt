# Generated from multistreamserver.pro.

cmake_minimum_required(VERSION 3.14)
project(multistreamserver LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(INSTALL_EXAMPLEDIR "examples")

find_package(Qt6 COMPONENTS Network)
find_package(Qt6 COMPONENTS Widgets)

add_qt_gui_executable(multistreamserver
    chatprovider.cpp chatprovider.h
    main.cpp
    movieprovider.cpp movieprovider.h
    provider.h
    server.cpp server.h
    timeprovider.cpp timeprovider.h
)
target_link_libraries(multistreamserver PUBLIC
    Qt::Network
    Qt::Widgets
)

install(TARGETS multistreamserver
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)

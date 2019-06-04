# Generated from threadedfortuneserver.pro.

cmake_minimum_required(VERSION 3.14)
project(threadedfortuneserver LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(INSTALL_EXAMPLEDIR "examples")

find_package(Qt5 COMPONENTS Network)
find_package(Qt5 COMPONENTS Widgets)

add_executable(threadedfortuneserver WIN32 MACOSX_BUNDLE
    dialog.cpp dialog.h
    fortuneserver.cpp fortuneserver.h
    fortunethread.cpp fortunethread.h
    main.cpp
)
target_link_libraries(threadedfortuneserver PUBLIC
    Qt::Network
    Qt::Widgets
)

install(TARGETS threadedfortuneserver
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
)

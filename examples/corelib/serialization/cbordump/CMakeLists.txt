# Generated from cbordump.pro.

cmake_minimum_required(VERSION 3.14)
project(cbordump LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(INSTALL_EXAMPLEDIR "examples/corelib/serialization/cbordump")

find_package(Qt6 COMPONENTS Core)

add_executable(cbordump
    main.cpp
)
target_link_libraries(cbordump PUBLIC
    Qt::Core
)

install(TARGETS cbordump
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)

# Generated from gradients.pro.

cmake_minimum_required(VERSION 3.14)
project(gradients LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(INSTALL_EXAMPLEDIR "examples")

find_package(Qt5 COMPONENTS Widgets)

add_executable(gradients WIN32 MACOSX_BUNDLE
    # special case: remove files from ../shared
    gradients.cpp gradients.h gradients.qrc
    main.cpp
)
target_include_directories(gradients PUBLIC
    ../shared
)
target_link_libraries(gradients PUBLIC
    Qt::Widgets
    painting_shared # special case
)

install(TARGETS gradients
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
)

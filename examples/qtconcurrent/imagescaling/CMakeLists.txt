# Generated from imagescaling.pro.

cmake_minimum_required(VERSION 3.14)
project(imagescaling LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
  set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/qtconcurrent/imagescaling")

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS Gui)
find_package(Qt6 COMPONENTS Concurrent)
find_package(Qt6 COMPONENTS Widgets)
find_package(Qt6 COMPONENTS Network)

qt_add_executable(imagescaling
    downloaddialog.cpp downloaddialog.h downloaddialog.ui
    imagescaling.cpp imagescaling.h
    main.cpp
)
set_target_properties(imagescaling PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)
target_link_libraries(imagescaling PUBLIC
    Qt::Concurrent
    Qt::Core
    Qt::Gui
    Qt::Network
    Qt::Widgets
)

install(TARGETS imagescaling
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)

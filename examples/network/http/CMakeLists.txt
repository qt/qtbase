# Generated from http.pro.

cmake_minimum_required(VERSION 3.14)
project(http LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
  set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/network/http")

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS Gui)
find_package(Qt6 COMPONENTS Network)
find_package(Qt6 COMPONENTS Widgets)

qt_add_executable(http
    authenticationdialog.ui
    httpwindow.cpp httpwindow.h
    main.cpp
)
set_target_properties(http PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)
target_link_libraries(http PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Network
    Qt::Widgets
)

if(ANDROID AND TARGET Qt::AndroidExtras)
    target_compile_definitions(http PUBLIC
        REQUEST_PERMISSIONS_ON_ANDROID
    )

    target_link_libraries(http PUBLIC
        Qt::AndroidExtras
    )
endif()

install(TARGETS http
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)

# Generated from windows.pro.

cmake_minimum_required(VERSION 3.14)
project(windows LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
  set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/qpa/windows")

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS Gui)

qt_add_executable(windows
    main.cpp
    window.cpp window.h
)
set_target_properties(windows PROPERTIES
    WIN32_EXECUTABLE FALSE
    MACOSX_BUNDLE TRUE
)
target_link_libraries(windows PUBLIC
    Qt::Core
    Qt::CorePrivate
    Qt::Gui
    Qt::GuiPrivate
)

install(TARGETS windows
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)

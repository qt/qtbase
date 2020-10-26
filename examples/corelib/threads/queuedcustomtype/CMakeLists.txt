# Generated from queuedcustomtype.pro.

cmake_minimum_required(VERSION 3.14)
project(queuedcustomtype LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
  set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/corelib/threads/queuedcustomtype")

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS Gui)
find_package(Qt6 COMPONENTS Widgets)

qt_add_executable(queuedcustomtype
    block.cpp block.h
    main.cpp
    renderthread.cpp renderthread.h
    window.cpp window.h
)
set_target_properties(queuedcustomtype PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)
target_link_libraries(queuedcustomtype PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Widgets
)

install(TARGETS queuedcustomtype
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)

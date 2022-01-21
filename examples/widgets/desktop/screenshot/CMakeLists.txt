cmake_minimum_required(VERSION 3.16)
project(screenshot LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/widgets/desktop/screenshot")

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)

qt_add_executable(screenshot
    main.cpp
    screenshot.cpp screenshot.h
)

set_target_properties(screenshot PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)

target_link_libraries(screenshot PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Widgets
)

install(TARGETS screenshot
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)

cmake_minimum_required(VERSION 3.16)
project(transformations LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/widgets/painting/transformations")

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)

qt_add_executable(transformations
    main.cpp
    renderarea.cpp renderarea.h
    window.cpp window.h
)

set_target_properties(transformations PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)

target_link_libraries(transformations PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Widgets
)

install(TARGETS transformations
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)

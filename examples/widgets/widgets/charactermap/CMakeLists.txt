cmake_minimum_required(VERSION 3.16)
project(charactermap LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/widgets/widgets/charactermap")

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)

qt_add_executable(charactermap
    characterwidget.cpp characterwidget.h
    main.cpp
    mainwindow.cpp mainwindow.h
)

set_target_properties(charactermap PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)

target_link_libraries(charactermap PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Widgets
)

install(TARGETS charactermap
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)

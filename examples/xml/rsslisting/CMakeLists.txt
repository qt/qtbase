cmake_minimum_required(VERSION 3.16)
project(rsslisting LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/xml/rsslisting")

find_package(Qt6 REQUIRED COMPONENTS Core Gui Network Widgets Xml)

qt_add_executable(rsslisting
    main.cpp
    rsslisting.cpp rsslisting.h
)

set_target_properties(rsslisting PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)

target_link_libraries(rsslisting PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Network
    Qt::Widgets
    Qt::Xml
)

install(TARGETS rsslisting
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)

cmake_minimum_required(VERSION 3.16)
project(customtype LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/corelib/tools/customtype")

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)

qt_add_executable(customtype
    main.cpp
    message.cpp message.h
)

set_target_properties(customtype PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)

target_link_libraries(customtype PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Widgets
)

install(TARGETS customtype
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)

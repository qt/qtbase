cmake_minimum_required(VERSION 3.16)
project(trivialwizard LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/widgets/dialogs/trivialwizard")

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)

qt_add_executable(trivialwizard
    trivialwizard.cpp
)

set_target_properties(trivialwizard PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)

target_link_libraries(trivialwizard PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Widgets
)

install(TARGETS trivialwizard
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)

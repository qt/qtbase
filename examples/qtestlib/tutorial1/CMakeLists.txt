cmake_minimum_required(VERSION 3.16)
project(tutorial1 LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/qtestlib/tutorial1")

find_package(Qt6 REQUIRED COMPONENTS Core Gui Test Widgets)

qt_add_executable(tutorial1
    testqstring.cpp
)

set_target_properties(tutorial1 PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)

target_link_libraries(tutorial1 PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Test
    Qt::Widgets
)

install(TARGETS tutorial1
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)

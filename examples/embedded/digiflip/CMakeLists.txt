cmake_minimum_required(VERSION 3.16)
project(digiflip LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/embedded/digiflip")

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)

qt_add_executable(digiflip
    digiflip.cpp
)

set_target_properties(digiflip PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)

target_link_libraries(digiflip PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Widgets
)

install(TARGETS digiflip
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)

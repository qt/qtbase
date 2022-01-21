cmake_minimum_required(VERSION 3.16)
project(fademessage LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/widgets/effects/fademessage")

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)

qt_add_executable(fademessage
    fademessage.cpp fademessage.h
    main.cpp
)

set_target_properties(fademessage PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)

target_link_libraries(fademessage PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Widgets
)

# Resources:
set(fademessage_resource_files
    "background.jpg"
)

qt_add_resources(fademessage "fademessage"
    PREFIX
        "/"
    FILES
        ${fademessage_resource_files}
)

install(TARGETS fademessage
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)

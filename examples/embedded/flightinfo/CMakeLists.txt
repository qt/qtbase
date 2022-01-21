cmake_minimum_required(VERSION 3.16)
project(flightinfo LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/embedded/flightinfo")

find_package(Qt6 REQUIRED COMPONENTS Core Gui Network Widgets)

qt_add_executable(flightinfo
    flightinfo.cpp
    form.ui
)

set_target_properties(flightinfo PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)

target_link_libraries(flightinfo PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Network
    Qt::Widgets
)

# Resources:
set(flightinfo_resource_files
    "aircraft.png"
)

qt_add_resources(flightinfo "flightinfo"
    PREFIX
        "/"
    FILES
        ${flightinfo_resource_files}
)

install(TARGETS flightinfo
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)

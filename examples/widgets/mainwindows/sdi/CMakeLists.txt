cmake_minimum_required(VERSION 3.16)
project(sdi LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/widgets/mainwindows/sdi")

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)

qt_add_executable(sdi
    main.cpp
    mainwindow.cpp mainwindow.h
)

set_target_properties(sdi PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)

target_link_libraries(sdi PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Widgets
)

# Resources:
set(sdi_resource_files
    "images/copy.png"
    "images/cut.png"
    "images/new.png"
    "images/open.png"
    "images/paste.png"
    "images/save.png"
)

qt_add_resources(sdi "sdi"
    PREFIX
        "/"
    FILES
        ${sdi_resource_files}
)

install(TARGETS sdi
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)

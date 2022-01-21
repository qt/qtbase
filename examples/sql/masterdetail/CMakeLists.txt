cmake_minimum_required(VERSION 3.16)
project(masterdetail LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/sql/masterdetail")

find_package(Qt6 REQUIRED COMPONENTS Core Gui Sql Widgets Xml)

qt_add_executable(masterdetail
    database.h
    dialog.cpp dialog.h
    main.cpp
    mainwindow.cpp mainwindow.h
)

set_target_properties(masterdetail PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)

target_link_libraries(masterdetail PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Sql
    Qt::Widgets
    Qt::Xml
)

# Resources:
set(masterdetail_resource_files
    "images/icon.png"
    "images/image.png"
)

qt_add_resources(masterdetail "masterdetail"
    PREFIX
        "/"
    FILES
        ${masterdetail_resource_files}
)

install(TARGETS masterdetail
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)

# Generated from licensewizard.pro.

cmake_minimum_required(VERSION 3.14)
project(licensewizard LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
  set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/widgets/dialogs/licensewizard")

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS Gui)
find_package(Qt6 COMPONENTS Widgets)
find_package(Qt6 COMPONENTS PrintSupport)

qt_add_executable(licensewizard
    licensewizard.cpp licensewizard.h
    main.cpp
)
set_target_properties(licensewizard PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)
target_link_libraries(licensewizard PUBLIC
    Qt::Core
    Qt::Gui
    Qt::PrintSupport
    Qt::Widgets
)


# Resources:
set(licensewizard_resource_files
    "images/logo.png"
    "images/watermark.png"
)

qt6_add_resources(licensewizard "licensewizard"
    PREFIX
        "/"
    FILES
        ${licensewizard_resource_files}
)

install(TARGETS licensewizard
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)

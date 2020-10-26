# Generated from completer.pro.

cmake_minimum_required(VERSION 3.14)
project(completer LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
  set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/widgets/tools/completer")

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS Gui)
find_package(Qt6 COMPONENTS Widgets)

qt_add_executable(completer
    fsmodel.cpp fsmodel.h
    main.cpp
    mainwindow.cpp mainwindow.h
)
set_target_properties(completer PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)
target_link_libraries(completer PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Widgets
)


# Resources:
set(completer_resource_files
    "resources/countries.txt"
    "resources/wordlist.txt"
)

qt6_add_resources(completer "completer"
    PREFIX
        "/"
    FILES
        ${completer_resource_files}
)

install(TARGETS completer
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)

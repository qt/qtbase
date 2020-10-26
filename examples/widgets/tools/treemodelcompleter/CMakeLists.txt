# Generated from treemodelcompleter.pro.

cmake_minimum_required(VERSION 3.14)
project(treemodelcompleter LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
  set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/widgets/tools/treemodelcompleter")

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS Gui)
find_package(Qt6 COMPONENTS Widgets)

qt_add_executable(treemodelcompleter
    main.cpp
    mainwindow.cpp mainwindow.h
    treemodelcompleter.cpp treemodelcompleter.h
)
set_target_properties(treemodelcompleter PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)
target_link_libraries(treemodelcompleter PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Widgets
)


# Resources:
set(treemodelcompleter_resource_files
    "resources/treemodel.txt"
)

qt6_add_resources(treemodelcompleter "treemodelcompleter"
    PREFIX
        "/"
    FILES
        ${treemodelcompleter_resource_files}
)

install(TARGETS treemodelcompleter
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)

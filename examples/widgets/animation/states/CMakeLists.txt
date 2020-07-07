# Generated from states.pro.

cmake_minimum_required(VERSION 3.14)
project(states LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
  set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/widgets/animation/states")

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS Gui)
find_package(Qt6 COMPONENTS Widgets)

add_qt_gui_executable(states
    main.cpp
)
target_link_libraries(states PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Widgets
)


# Resources:
set(states_resource_files
    "accessories-dictionary.png"
    "akregator.png"
    "digikam.png"
    "help-browser.png"
    "k3b.png"
    "kchart.png"
)

qt6_add_resources(states "states"
    PREFIX
        "/"
    FILES
        ${states_resource_files}
)

install(TARGETS states
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)

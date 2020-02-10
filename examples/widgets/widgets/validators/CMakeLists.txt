# Generated from validators.pro.

cmake_minimum_required(VERSION 3.14)
project(validators LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(INSTALL_EXAMPLEDIR "examples/widgets/widgets/validators")

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS Gui)
find_package(Qt6 COMPONENTS Widgets)

add_qt_gui_executable(validators
    ledwidget.cpp ledwidget.h
    localeselector.cpp localeselector.h
    main.cpp
    validators.ui
    validatorwidget.cpp validatorwidget.h
)
target_link_libraries(validators PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Widgets
)


# Resources:
set(validators_resource_files
    "ledoff.png"
    "ledon.png"
)

qt6_add_resources(validators "validators"
    PREFIX
        "/"
    FILES
        ${validators_resource_files}
)

install(TARGETS validators
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)

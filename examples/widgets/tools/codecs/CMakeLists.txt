# Generated from codecs.pro.

cmake_minimum_required(VERSION 3.14)
project(codecs LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(INSTALL_EXAMPLEDIR "examples/widgets/tools/codecs")

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS Gui)
find_package(Qt6 COMPONENTS Widgets)

add_qt_gui_executable(codecs
    encodingdialog.cpp encodingdialog.h
    main.cpp
    mainwindow.cpp mainwindow.h
    previewform.cpp previewform.h
)
target_link_libraries(codecs PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Widgets
)


# Resources:
set(codecs_resource_files
    "images/editcopy.png"
)

qt6_add_resources(codecs "codecs"
    PREFIX
        "/"
    FILES
        ${codecs_resource_files}
)

install(TARGETS codecs
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)

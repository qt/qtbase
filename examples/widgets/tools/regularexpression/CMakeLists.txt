# Generated from regularexpression.pro.

cmake_minimum_required(VERSION 3.14)
project(regularexpression LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
  set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/widgets/tools/regularexpression")

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS Gui)
find_package(Qt6 COMPONENTS Widgets)

qt_add_executable(regularexpression
    main.cpp
    regularexpressiondialog.cpp regularexpressiondialog.h
)
set_target_properties(regularexpression PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)
target_link_libraries(regularexpression PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Widgets
)


# Resources:
set(regularexpression_resource_files
    "images/copy.png"
)

qt6_add_resources(regularexpression "regularexpression"
    PREFIX
        "/"
    FILES
        ${regularexpression_resource_files}
)

install(TARGETS regularexpression
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)

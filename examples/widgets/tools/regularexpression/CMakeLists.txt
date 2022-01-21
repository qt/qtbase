cmake_minimum_required(VERSION 3.16)
project(regularexpression LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/widgets/tools/regularexpression")

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)

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

qt_add_resources(regularexpression "regularexpression"
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

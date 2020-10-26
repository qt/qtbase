# Generated from interview.pro.

cmake_minimum_required(VERSION 3.14)
project(interview LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
  set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/widgets/itemviews/interview")

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS Gui)
find_package(Qt6 COMPONENTS Widgets)

qt_add_executable(interview
    main.cpp
    model.cpp model.h
)
set_target_properties(interview PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)
target_link_libraries(interview PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Widgets
)


# Resources:
set(interview_resource_files
    "images/folder.png"
    "images/interview.png"
    "images/services.png"
)

qt6_add_resources(interview "interview"
    PREFIX
        "/"
    FILES
        ${interview_resource_files}
)

install(TARGETS interview
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)

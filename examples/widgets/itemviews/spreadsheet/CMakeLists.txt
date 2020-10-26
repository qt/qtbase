# Generated from spreadsheet.pro.

cmake_minimum_required(VERSION 3.14)
project(spreadsheet LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
  set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/widgets/itemviews/spreadsheet")

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS Gui)
find_package(Qt6 COMPONENTS Widgets)
find_package(Qt6 COMPONENTS PrintSupport) # special case

qt_add_executable(spreadsheet
    main.cpp
    printview.cpp printview.h
    spreadsheet.cpp spreadsheet.h
    spreadsheetdelegate.cpp spreadsheetdelegate.h
    spreadsheetitem.cpp spreadsheetitem.h
)
set_target_properties(spreadsheet PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)
target_link_libraries(spreadsheet PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Widgets
)
# special case begin
if (TARGET Qt::PrintSupport)
    target_link_libraries(spreadsheet PUBLIC Qt::PrintSupport)
endif()
# special case end


# Resources:
set(spreadsheet_resource_files
    "images/interview.png"
)

qt6_add_resources(spreadsheet "spreadsheet"
    PREFIX
        "/"
    FILES
        ${spreadsheet_resource_files}
)

if(TARGET Qt::PrintSupport)
    target_link_libraries(spreadsheet PUBLIC
        Qt::PrintSupport
    )
endif()

install(TARGETS spreadsheet
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)

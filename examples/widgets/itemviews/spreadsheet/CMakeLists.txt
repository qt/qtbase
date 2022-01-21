cmake_minimum_required(VERSION 3.16)
project(spreadsheet LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/widgets/itemviews/spreadsheet")

find_package(Qt6
    REQUIRED COMPONENTS Core Gui Widgets
    OPTIONAL_COMPONENTS PrintSupport
)

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

if (TARGET Qt::PrintSupport)
    target_link_libraries(spreadsheet PUBLIC Qt::PrintSupport)
endif()

# Resources:
set(spreadsheet_resource_files
    "images/interview.png"
)

qt_add_resources(spreadsheet "spreadsheet"
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

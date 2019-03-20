# Generated from spreadsheet.pro.

#####################################################################
## spreadsheet Binary:
#####################################################################

add_qt_executable(spreadsheet
    GUI
    OUTPUT_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/itemviews/spreadsheet"
    INSTALL_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/itemviews/spreadsheet"
    SOURCES
        main.cpp
        printview.cpp printview.h
        spreadsheet.cpp spreadsheet.h
        spreadsheetdelegate.cpp spreadsheetdelegate.h
        spreadsheetitem.cpp spreadsheetitem.h
    LIBRARIES
        Qt::Widgets
)

# Resources:
add_qt_resource(spreadsheet "spreadsheet" PREFIX "/" FILES
    images/interview.png)


## Scopes:
#####################################################################

extend_target(spreadsheet CONDITION TARGET Qt::PrintSupport
    LIBRARIES
        Qt::PrintSupport
)

#### Keys ignored in scope 3:.:spreadsheet.pro:build_all AND NOT build_pass:
# CONFIG = "-build_all" "release"

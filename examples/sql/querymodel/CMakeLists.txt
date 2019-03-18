# Generated from querymodel.pro.

#####################################################################
## querymodel Binary:
#####################################################################

add_qt_executable(querymodel
    GUI
    OUTPUT_DIRECTORY "${INSTALL_EXAMPLESDIR}/sql/querymodel"
    INSTALL_DIRECTORY "${INSTALL_EXAMPLESDIR}/sql/querymodel"
    SOURCES
        ../connection.h
        customsqlmodel.cpp customsqlmodel.h
        editablesqlmodel.cpp editablesqlmodel.h
        main.cpp
    LIBRARIES
        Qt::Sql
        Qt::Widgets
)

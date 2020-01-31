#####################################################################
## moc Tool:
#####################################################################

qt_add_tool(cmake_automoc_parser
    BOOTSTRAP
    TOOLS_TARGET Core # special case
    SOURCES
        main.cpp
    DEFINES
        QT_MOC
        QT_NO_CAST_FROM_ASCII
        QT_NO_CAST_FROM_BYTEARRAY
        QT_NO_COMPRESS
        QT_NO_FOREACH
    INCLUDE_DIRECTORIES
        ${CMAKE_CURRENT_SOURCE_DIR}
)

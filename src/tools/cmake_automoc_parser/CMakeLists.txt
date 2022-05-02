#####################################################################
## moc Tool:
#####################################################################

qt_get_tool_target_name(target_name cmake_automoc_parser)
qt_internal_add_tool(${target_name}
    CORE_LIBRARY Bootstrap
    INSTALL_DIR "${INSTALL_LIBEXECDIR}"
    TOOLS_TARGET Core # special case
    SOURCES
        main.cpp
    DEFINES
        QT_MOC
        QT_NO_CAST_FROM_ASCII
        QT_NO_CAST_FROM_BYTEARRAY
        QT_NO_FOREACH
    INCLUDE_DIRECTORIES
        ${CMAKE_CURRENT_SOURCE_DIR}
)
qt_internal_return_unless_building_tools()

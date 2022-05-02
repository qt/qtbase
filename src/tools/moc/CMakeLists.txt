# Generated from moc.pro.

#####################################################################
## moc Tool:
#####################################################################

qt_get_tool_target_name(target_name moc)
qt_internal_add_tool(${target_name}
    CORE_LIBRARY Bootstrap
    TARGET_DESCRIPTION "Qt Meta Object Compiler"
    INSTALL_DIR "${INSTALL_LIBEXECDIR}"
    TOOLS_TARGET Core # special case
    SOURCES
        cbordevice.h
        collectjson.cpp collectjson.h
        generator.cpp generator.h
        main.cpp
        moc.cpp moc.h
        outputrevision.h
        parser.cpp parser.h
        preprocessor.cpp preprocessor.h
        # qdatetime_p.h special case remove
        symbols.h
        token.cpp token.h
        utils.h
    DEFINES
        QT_MOC
        QT_NO_CAST_FROM_ASCII
        QT_NO_CAST_FROM_BYTEARRAY
        QT_NO_FOREACH
    INCLUDE_DIRECTORIES
        ${CMAKE_CURRENT_SOURCE_DIR}
        ../../3rdparty/tinycbor/src
        ../shared
)
qt_internal_return_unless_building_tools()

#### Keys ignored in scope 1:.:.:moc.pro:<TRUE>:
# QMAKE_TARGET_DESCRIPTION = "Qt Meta Object Compiler"
# _OPTION = "host_build"

## Scopes:
#####################################################################

qt_internal_extend_target(${target_name} CONDITION MSVC
    DEFINES
        _CRT_SECURE_NO_WARNINGS
)

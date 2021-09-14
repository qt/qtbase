# Generated from qdbuscpp2xml.pro.

#####################################################################
## qdbuscpp2xml Tool:
#####################################################################

qt_get_tool_target_name(target_name qdbuscpp2xml)
qt_internal_add_tool(${target_name}
    TARGET_DESCRIPTION "Qt D-Bus C++ to XML Compiler"
    TOOLS_TARGET DBus # special case
    SOURCES
        ../moc/cbordevice.h
        ../moc/collectjson.cpp ../moc/collectjson.h
        ../moc/generator.cpp ../moc/generator.h
        ../moc/moc.cpp ../moc/moc.h
        ../moc/outputrevision.h
        ../moc/parser.cpp ../moc/parser.h
        ../moc/preprocessor.cpp ../moc/preprocessor.h
        ../moc/symbols.h
        ../moc/token.cpp ../moc/token.h
        ../moc/utils.h
        qdbuscpp2xml.cpp
    DEFINES
        QT_NO_CAST_FROM_ASCII
        QT_NO_FOREACH
    INCLUDE_DIRECTORIES
        ../moc
        ../moc/../../3rdparty/tinycbor/src
    PUBLIC_LIBRARIES
        Qt::CorePrivate
        Qt::DBusPrivate
    # COMPILE_OPTIONS  # special case
    #     "$$QT_HOST_CFLAGS_DBUS"
    # QMAKE_TARGET_DESCRIPTION = "Qt D-Bus C++ to XML Compiler"
    # _LOADED = "qt_tool"
    # _OPTION = "host_build"
)
qt_internal_return_unless_building_tools()

#### Keys ignored in scope 1:.:.:qdbuscpp2xml.pro:<TRUE>:
# QMAKE_TARGET_DESCRIPTION = "Qt D-Bus C++ to XML Compiler"
# _OPTION = "host_build"

## Scopes:
#####################################################################

# special case begin
# qt_internal_extend_target(qdbuscpp2xml CONDITION force_bootstrap [...])
# qt_internal_extend_target(qdbuscpp2xml CONDITION NOT force_bootstrap [...])
# special case end

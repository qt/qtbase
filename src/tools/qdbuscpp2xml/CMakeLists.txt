# Generated from qdbuscpp2xml.pro.

#####################################################################
## qdbuscpp2xml Tool:
#####################################################################

add_qt_tool(qdbuscpp2xml
    SOURCES
        ../moc/cbordevice.h
        ../moc/generator.cpp ../moc/generator.h
        ../moc/moc.cpp ../moc/moc.h
        ../moc/outputrevision.h
        ../moc/parser.cpp ../moc/parser.h
        ../moc/preprocessor.cpp ../moc/preprocessor.h
        ../moc/symbols.h
        ../moc/token.cpp ../moc/token.h
        ../moc/utils.h
        qdbuscpp2xml.cpp  # special case
    DEFINES
        QT_NO_CAST_FROM_ASCII
        QT_NO_FOREACH
    INCLUDE_DIRECTORIES
        ../moc
    LIBRARIES
        tinycbor  # special case
        Qt::CorePrivate
        Qt::DBusPrivate
    # COMPILE_OPTIONS  # special case
    #     "$$QT_HOST_CFLAGS_DBUS"
    # QMAKE_TARGET_DESCRIPTION = "Qt D-Bus C++ to XML Compiler"
    # _LOADED = "qt_tool"
    # _OPTION = "host_build"
)

## Scopes:
#####################################################################

# special case:
# extend_target(qdbuscpp2xml CONDITION force_bootstrap [...])
# extend_target(qdbuscpp2xml CONDITION NOT force_bootstrap [...])

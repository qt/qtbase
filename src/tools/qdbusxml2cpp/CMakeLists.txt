# Generated from qdbusxml2cpp.pro.

#####################################################################
## qdbusxml2cpp Tool:
#####################################################################

qt_get_tool_target_name(target_name qdbusxml2cpp)
qt_internal_add_tool(${target_name}
    TARGET_DESCRIPTION "Qt D-Bus XML to C++ Compiler"
    TOOLS_TARGET DBus # special case
    SOURCES
        qdbusxml2cpp.cpp
    DEFINES
        QT_NO_CAST_FROM_ASCII
        QT_NO_FOREACH
    PUBLIC_LIBRARIES
        Qt::CorePrivate
        Qt::DBusPrivate
    # COMPILE_OPTIONS  # special case
    #     "$$QT_HOST_CFLAGS_DBUS"
    # QMAKE_TARGET_DESCRIPTION = "Qt D-Bus XML to C++ Compiler"
    # _LOADED = "qt_tool"
    # _OPTION = "host_build"
)

#### Keys ignored in scope 1:.:.:qdbusxml2cpp.pro:<TRUE>:
# QMAKE_TARGET_DESCRIPTION = "Qt D-Bus XML to C++ Compiler"
# _OPTION = "host_build"

## Scopes:
#####################################################################

# special case begin
# qt_internal_extend_target(qdbusxml2cpp CONDITION NOT force_bootstrap [...])
# qt_internal_extend_target(qdbusxml2cpp CONDITION NOT QT_FEATURE_commandlineparser AND NOT force_bootstrap [...])
# qt_internal_extend_target(qdbusxml2cpp CONDITION force_bootstrap [...])
# special case end

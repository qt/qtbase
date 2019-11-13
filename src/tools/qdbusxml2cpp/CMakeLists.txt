# Generated from qdbusxml2cpp.pro.

#####################################################################
## qdbusxml2cpp Tool:
#####################################################################

qt_add_tool(qdbusxml2cpp
    TOOLS_TARGET DBus # special case
    SOURCES
        qdbusxml2cpp.cpp
    DEFINES
        QT_NO_CAST_FROM_ASCII
        QT_NO_FOREACH
    LIBRARIES
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
# extend_target(qdbusxml2cpp CONDITION NOT force_bootstrap [...])
# extend_target(qdbusxml2cpp CONDITION NOT QT_FEATURE_commandlineparser AND NOT force_bootstrap [...])
# extend_target(qdbusxml2cpp CONDITION force_bootstrap [...])
# special case end

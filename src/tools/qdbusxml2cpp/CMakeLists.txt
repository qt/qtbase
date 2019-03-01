# Generated from qdbusxml2cpp.pro.

#####################################################################
## qdbusxml2cpp Tool:
#####################################################################

add_qt_tool(qdbusxml2cpp
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

## Scopes:
#####################################################################

# special case:
# extend_target(qdbusxml2cpp CONDITION NOT force_bootstrap [...])
#extend_target(qdbusxml2cpp CONDITION NOT QT_FEATURE_commandlineparser AND NOT force_bootstrap [...])
# extend_target(qdbusxml2cpp CONDITION force_bootstrap [...])

# Generated from evdevmouse.pro.

#####################################################################
## qevdevmouseplugin Plugin:
#####################################################################

add_qt_plugin(qevdevmouseplugin
    TYPE generic
    SOURCES
        main.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::GuiPrivate
        Qt::InputSupportPrivate
    # OTHER_FILES = "evdevmouse.json"
    # PLUGIN_CLASS_NAME = "QEvdevMousePlugin"
    # PLUGIN_EXTENDS = "-"
    # _LOADED = "qt_plugin"
)

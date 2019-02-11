# Generated from evdevkeyboard.pro.

#####################################################################
## qevdevkeyboardplugin Plugin:
#####################################################################

add_qt_plugin(qevdevkeyboardplugin
    TYPE generic
    SOURCES
        main.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::GuiPrivate
        Qt::InputSupportPrivate
    # OTHER_FILES = "evdevkeyboard.json"
    # PLUGIN_CLASS_NAME = "QEvdevKeyboardPlugin"
    # PLUGIN_EXTENDS = "-"
    # _LOADED = "qt_plugin"
)

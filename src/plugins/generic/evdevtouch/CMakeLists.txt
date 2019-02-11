# Generated from evdevtouch.pro.

#####################################################################
## qevdevtouchplugin Plugin:
#####################################################################

add_qt_plugin(qevdevtouchplugin
    TYPE generic
    SOURCES
        main.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::GuiPrivate
        Qt::InputSupportPrivate
    # OTHER_FILES = "evdevtouch.json"
    # PLUGIN_CLASS_NAME = "QEvdevTouchScreenPlugin"
    # PLUGIN_EXTENDS = "-"
    # _LOADED = "qt_plugin"
)

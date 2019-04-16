# Generated from evdevtablet.pro.

#####################################################################
## qevdevtabletplugin Plugin:
#####################################################################

add_qt_plugin(qevdevtabletplugin
    TYPE generic
    SOURCES
        main.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::GuiPrivate
        Qt::InputSupportPrivate
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::Gui
        Qt::InputSupport
)

#### Keys ignored in scope 1:.:.:evdevtablet.pro:<TRUE>:
# OTHER_FILES = "evdevtablet.json"
# PLUGIN_CLASS_NAME = "QEvdevTabletPlugin"
# PLUGIN_EXTENDS = "-"
# _LOADED = "qt_plugin"

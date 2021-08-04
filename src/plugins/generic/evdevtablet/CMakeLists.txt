# Generated from evdevtablet.pro.

#####################################################################
## QEvdevTabletPlugin Plugin:
#####################################################################

qt_internal_add_plugin(QEvdevTabletPlugin
    OUTPUT_NAME qevdevtabletplugin
    PLUGIN_TYPE generic
    DEFAULT_IF FALSE
    SOURCES
        main.cpp
    LIBRARIES
        Qt::Core
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
        Qt::InputSupportPrivate
)

#### Keys ignored in scope 1:.:.:evdevtablet.pro:<TRUE>:
# OTHER_FILES = "evdevtablet.json"
# PLUGIN_EXTENDS = "-"

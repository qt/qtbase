# Generated from evdevtablet.pro.

#####################################################################
## QEvdevTabletPlugin Plugin:
#####################################################################

qt_add_plugin(QEvdevTabletPlugin
    OUTPUT_NAME qevdevtabletplugin
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
# PLUGIN_EXTENDS = "-"

# Generated from evdevmouse.pro.

#####################################################################
## QEvdevMousePlugin Plugin:
#####################################################################

qt_internal_add_plugin(QEvdevMousePlugin
    OUTPUT_NAME qevdevmouseplugin
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

#### Keys ignored in scope 1:.:.:evdevmouse.pro:<TRUE>:
# OTHER_FILES = "evdevmouse.json"
# PLUGIN_EXTENDS = "-"

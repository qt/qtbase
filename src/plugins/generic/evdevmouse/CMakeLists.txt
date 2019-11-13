# Generated from evdevmouse.pro.

#####################################################################
## qevdevmouseplugin Plugin:
#####################################################################

qt_add_plugin(qevdevmouseplugin
    TYPE generic
    CLASS_NAME QEvdevMousePlugin
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

#### Keys ignored in scope 1:.:.:evdevmouse.pro:<TRUE>:
# OTHER_FILES = "evdevmouse.json"
# PLUGIN_EXTENDS = "-"

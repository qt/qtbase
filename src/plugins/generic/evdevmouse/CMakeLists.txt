# Generated from evdevmouse.pro.

#####################################################################
## QEvdevMousePlugin Plugin:
#####################################################################

qt_add_plugin(QEvdevMousePlugin
    OUTPUT_NAME qevdevmouseplugin
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

#### Keys ignored in scope 1:.:.:evdevmouse.pro:<TRUE>:
# OTHER_FILES = "evdevmouse.json"
# PLUGIN_EXTENDS = "-"

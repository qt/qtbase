# Generated from evdevtouch.pro.

#####################################################################
## qevdevtouchplugin Plugin:
#####################################################################

add_qt_plugin(qevdevtouchplugin
    TYPE generic
    CLASS_NAME QEvdevTouchScreenPlugin
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

#### Keys ignored in scope 1:.:.:evdevtouch.pro:<TRUE>:
# OTHER_FILES = "evdevtouch.json"
# PLUGIN_EXTENDS = "-"

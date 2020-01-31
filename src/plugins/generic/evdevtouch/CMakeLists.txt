# Generated from evdevtouch.pro.

#####################################################################
## QEvdevTouchScreenPlugin Plugin:
#####################################################################

qt_add_plugin(QEvdevTouchScreenPlugin
    OUTPUT_NAME qevdevtouchplugin
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

#### Keys ignored in scope 1:.:.:evdevtouch.pro:<TRUE>:
# OTHER_FILES = "evdevtouch.json"
# PLUGIN_EXTENDS = "-"

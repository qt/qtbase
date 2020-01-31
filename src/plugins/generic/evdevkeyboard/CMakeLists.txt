# Generated from evdevkeyboard.pro.

#####################################################################
## QEvdevKeyboardPlugin Plugin:
#####################################################################

qt_add_plugin(QEvdevKeyboardPlugin
    OUTPUT_NAME qevdevkeyboardplugin
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

#### Keys ignored in scope 1:.:.:evdevkeyboard.pro:<TRUE>:
# OTHER_FILES = "evdevkeyboard.json"
# PLUGIN_EXTENDS = "-"

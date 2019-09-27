# Generated from libinput.pro.

#####################################################################
## qlibinputplugin Plugin:
#####################################################################

add_qt_plugin(qlibinputplugin
    TYPE generic
    CLASS_NAME QLibInputPlugin
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

#### Keys ignored in scope 1:.:.:libinput.pro:<TRUE>:
# OTHER_FILES = "libinput.json"
# PLUGIN_CLASS_NAME = "QLibInputPlugin"
# PLUGIN_EXTENDS = "-"
# _LOADED = "qt_plugin"

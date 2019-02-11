# Generated from libinput.pro.

#####################################################################
## qlibinputplugin Plugin:
#####################################################################

add_qt_plugin(qlibinputplugin
    TYPE generic
    SOURCES
        main.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::GuiPrivate
        Qt::InputSupportPrivate
    # OTHER_FILES = "libinput.json"
    # PLUGIN_CLASS_NAME = "QLibInputPlugin"
    # PLUGIN_EXTENDS = "-"
    # _LOADED = "qt_plugin"
)

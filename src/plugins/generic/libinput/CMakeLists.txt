# Generated from libinput.pro.

#####################################################################
## QLibInputPlugin Plugin:
#####################################################################

qt_internal_add_plugin(QLibInputPlugin
    OUTPUT_NAME qlibinputplugin
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

#### Keys ignored in scope 1:.:.:libinput.pro:<TRUE>:
# OTHER_FILES = "libinput.json"
# PLUGIN_EXTENDS = "-"

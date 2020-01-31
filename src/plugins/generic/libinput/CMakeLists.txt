# Generated from libinput.pro.

#####################################################################
## QLibInputPlugin Plugin:
#####################################################################

qt_add_plugin(QLibInputPlugin
    OUTPUT_NAME qlibinputplugin
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

#### Keys ignored in scope 1:.:.:libinput.pro:<TRUE>:
# OTHER_FILES = "libinput.json"
# PLUGIN_EXTENDS = "-"

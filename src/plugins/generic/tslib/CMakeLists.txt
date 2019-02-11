# Generated from tslib.pro.

#####################################################################
## qtslibplugin Plugin:
#####################################################################

add_qt_plugin(qtslibplugin
    TYPE generic
    SOURCES
        main.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::GuiPrivate
        Qt::InputSupportPrivate
    # OTHER_FILES = "tslib.json"
    # PLUGIN_CLASS_NAME = "QTsLibPlugin"
    # PLUGIN_EXTENDS = "-"
    # _LOADED = "qt_plugin"
)

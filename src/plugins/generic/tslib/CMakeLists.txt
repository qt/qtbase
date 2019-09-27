# Generated from tslib.pro.

qt_find_package(Tslib) # special case

#####################################################################
## qtslibplugin Plugin:
#####################################################################

add_qt_plugin(qtslibplugin
    TYPE generic
    CLASS_NAME QTsLibPlugin
    SOURCES
        main.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::GuiPrivate
        Qt::InputSupportPrivate
    PUBLIC_LIBRARIES
        PkgConfig::Tslib
        Qt::Core
        Qt::Gui
        Qt::InputSupport
)

#### Keys ignored in scope 1:.:.:tslib.pro:<TRUE>:
# OTHER_FILES = "tslib.json"
# PLUGIN_CLASS_NAME = "QTsLibPlugin"
# PLUGIN_EXTENDS = "-"
# _LOADED = "qt_plugin"

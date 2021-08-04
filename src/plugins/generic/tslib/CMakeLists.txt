# Generated from tslib.pro.

qt_find_package(Tslib) # special case

#####################################################################
## QTsLibPlugin Plugin:
#####################################################################

qt_internal_add_plugin(QTsLibPlugin
    OUTPUT_NAME qtslibplugin
    PLUGIN_TYPE generic
    DEFAULT_IF FALSE
    SOURCES
        main.cpp
    LIBRARIES
        PkgConfig::Tslib
        Qt::Core
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
        Qt::InputSupportPrivate
)

#### Keys ignored in scope 1:.:.:tslib.pro:<TRUE>:
# OTHER_FILES = "tslib.json"
# PLUGIN_EXTENDS = "-"

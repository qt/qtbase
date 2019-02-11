# Generated from ibus.pro.

#####################################################################
## ibusplatforminputcontextplugin Plugin:
#####################################################################

add_qt_plugin(ibusplatforminputcontextplugin
    TYPE platforminputcontexts
    SOURCES
        main.cpp
        qibusinputcontextproxy.cpp qibusinputcontextproxy.h
        qibusplatforminputcontext.cpp qibusplatforminputcontext.h
        qibusproxy.cpp qibusproxy.h
        qibusproxyportal.cpp qibusproxyportal.h
        qibustypes.cpp qibustypes.h
    LIBRARIES
        Qt::DBus
        Qt::GuiPrivate
    # OTHER_FILES = "$$PWD/ibus.json"
    # PLUGIN_CLASS_NAME = "QIbusPlatformInputContextPlugin"
    # PLUGIN_EXTENDS = "-"
    # _LOADED = "qt_plugin"
)

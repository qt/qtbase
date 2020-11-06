# Generated from ibus.pro.

#####################################################################
## QIbusPlatformInputContextPlugin Plugin:
#####################################################################

qt_internal_add_plugin(QIbusPlatformInputContextPlugin
    OUTPUT_NAME ibusplatforminputcontextplugin
    TYPE platforminputcontexts
    DEFAULT_IF FALSE
    SOURCES
        main.cpp
        qibusinputcontextproxy.cpp qibusinputcontextproxy.h
        qibusplatforminputcontext.cpp qibusplatforminputcontext.h
        qibusproxy.cpp qibusproxy.h
        qibusproxyportal.cpp qibusproxyportal.h
        qibustypes.cpp qibustypes.h
    LIBRARIES
        XKB::XKB
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::DBus
        Qt::Gui
        Qt::GuiPrivate
)

#### Keys ignored in scope 1:.:.:ibus.pro:<TRUE>:
# OTHER_FILES = "$$PWD/ibus.json"
# PLUGIN_EXTENDS = "-"

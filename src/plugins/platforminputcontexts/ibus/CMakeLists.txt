# Generated from ibus.pro.

#####################################################################
## ibusplatforminputcontextplugin Plugin:
#####################################################################

add_qt_plugin(ibusplatforminputcontextplugin
    TYPE platforminputcontexts
    CLASS_NAME QIbusPlatformInputContextPlugin
    SOURCES
        main.cpp
        qibusinputcontextproxy.cpp qibusinputcontextproxy.h
        qibusplatforminputcontext.cpp qibusplatforminputcontext.h
        qibusproxy.cpp qibusproxy.h
        qibusproxyportal.cpp qibusproxyportal.h
        qibustypes.cpp qibustypes.h
    LIBRARIES
        Qt::GuiPrivate
        Qt::XkbCommonSupportPrivate
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::DBus
        Qt::Gui
        Qt::XkbCommonSupport
)

#### Keys ignored in scope 1:.:.:ibus.pro:<TRUE>:
# OTHER_FILES = "$$PWD/ibus.json"
# PLUGIN_EXTENDS = "-"

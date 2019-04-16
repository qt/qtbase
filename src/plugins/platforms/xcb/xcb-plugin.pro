TARGET = qxcb

QT += core-private gui-private xcb_qpa_lib-private

DEFINES += QT_NO_FOREACH

macos: CONFIG += no_app_extension_api_only

SOURCES = \
    qxcbmain.cpp

OTHER_FILES += xcb.json README

PLUGIN_TYPE = platforms
PLUGIN_CLASS_NAME = QXcbIntegrationPlugin
!equals(TARGET, $$QT_DEFAULT_QPA_PLUGIN): PLUGIN_EXTENDS = -
load(qt_plugin)

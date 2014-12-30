TARGET = qxcb

PLUGIN_TYPE = platforms
PLUGIN_CLASS_NAME = QXcbIntegrationPlugin
!equals(TARGET, $$QT_DEFAULT_QPA_PLUGIN): PLUGIN_EXTENDS = -
load(qt_plugin)

QT += core-private gui-private platformsupport-private xcb_qpa_lib-private

SOURCES = \
    qxcbmain.cpp
OTHER_FILES += xcb.json README


TEMPLATE = subdirs
CONFIG += ordered
QT_FOR_CONFIG += gui-private

!qtConfig(system-xcb): SUBDIRS += xcb-static

SUBDIRS += xcb_qpa_lib.pro
SUBDIRS += xcb-plugin.pro
SUBDIRS += gl_integrations

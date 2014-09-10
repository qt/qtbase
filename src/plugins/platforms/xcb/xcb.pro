TEMPLATE = subdirs
CONFIG += ordered

contains(QT_CONFIG, xcb-qt):SUBDIRS+=xcb-static

SUBDIRS += xcb_qpa_lib.pro
SUBDIRS += xcb-plugin.pro
SUBDIRS += gl_integrations

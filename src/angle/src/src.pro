TEMPLATE = subdirs
SUBDIRS += compiler

QT_FOR_CONFIG += gui

# needed as angle is built before gui
include($$OUT_PWD/../../gui/qtgui-config.pri)

qtConfig(combined-angle-lib): SUBDIRS += QtANGLE
else: SUBDIRS += libGLESv2 libEGL
CONFIG += ordered

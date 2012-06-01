TARGET = qevdevtabletplugin
load(qt_plugin)

DESTDIR = $$QT.gui.plugins/generic
target.path = $$[QT_INSTALL_PLUGINS]/generic
INSTALLS += target

SOURCES = main.cpp

QT += core-private platformsupport-private

OTHER_FILES += \
    evdevtablet.json

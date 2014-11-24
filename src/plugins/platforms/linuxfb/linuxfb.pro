TARGET = qlinuxfb

PLUGIN_TYPE = platforms
PLUGIN_CLASS_NAME = QLinuxFbIntegrationPlugin
!equals(TARGET, $$QT_DEFAULT_QPA_PLUGIN): PLUGIN_EXTENDS = -
load(qt_plugin)

QT += core-private gui-private platformsupport-private

SOURCES = main.cpp qlinuxfbintegration.cpp qlinuxfbscreen.cpp
HEADERS = qlinuxfbintegration.h qlinuxfbscreen.h
contains(QT_CONFIG, libudev): DEFINES += QDEVICEDISCOVERY_UDEV

CONFIG += qpa/genericunixfontdatabase

OTHER_FILES += linuxfb.json

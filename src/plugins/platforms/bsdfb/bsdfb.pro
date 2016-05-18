TARGET = qbsdfb

QT += core-private gui-private platformsupport-private

SOURCES = main.cpp qbsdfbintegration.cpp qbsdfbscreen.cpp
HEADERS = qbsdfbintegration.h qbsdfbscreen.h

CONFIG += qpa/genericunixfontdatabase

OTHER_FILES += bsdfb.json

PLUGIN_TYPE = platforms
PLUGIN_CLASS_NAME = QBsdFbIntegrationPlugin
!equals(TARGET, $$QT_DEFAULT_QPA_PLUGIN): PLUGIN_EXTENDS = -
load(qt_plugin)

TARGET = qbsdfb

QT += \
    core-private gui-private \
    service_support-private eventdispatcher_support-private \
    fontdatabase_support-private fb_support-private

qtHaveModule(input_support-private): \
    QT += input_support-private

SOURCES = main.cpp qbsdfbintegration.cpp qbsdfbscreen.cpp
HEADERS = qbsdfbintegration.h qbsdfbscreen.h

OTHER_FILES += bsdfb.json

PLUGIN_TYPE = platforms
PLUGIN_CLASS_NAME = QBsdFbIntegrationPlugin
!equals(TARGET, $$QT_DEFAULT_QPA_PLUGIN): PLUGIN_EXTENDS = -
load(qt_plugin)

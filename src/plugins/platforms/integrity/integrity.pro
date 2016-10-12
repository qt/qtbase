TARGET = integrityfb

QT += \
    core-private gui-private \
    eventdispatcher_support-private service_support-private \
    fontdatabase_support-private fb_support-private

SOURCES = \
    main.cpp \
    qintegrityfbintegration.cpp \
    qintegrityfbscreen.cpp \
    qintegrityhidmanager.cpp

HEADERS = \
    qintegrityfbintegration.h \
    qintegrityfbscreen.h \
    qintegrityhidmanager.h

OTHER_FILES += integrity.json

PLUGIN_TYPE = platforms
PLUGIN_CLASS_NAME = QIntegrityFbIntegrationPlugin
!equals(TARGET, $$QT_DEFAULT_QPA_PLUGIN): PLUGIN_EXTENDS = -
load(qt_plugin)

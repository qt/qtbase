TARGET = qvnc

PLUGIN_TYPE = platforms
PLUGIN_CLASS_NAME = QVncIntegrationPlugin
!equals(TARGET, $$QT_DEFAULT_QPA_PLUGIN): PLUGIN_EXTENDS = -
load(qt_plugin)

QT += \
    core-private network gui-private \
    service_support-private theme_support-private fb_support-private \
    eventdispatcher_support-private fontdatabase_support-private

qtHaveModule(input_support-private): \
    QT += input_support-private

DEFINES += QT_NO_FOREACH

SOURCES = \
    main.cpp \
    qvncintegration.cpp \
    qvncscreen.cpp \
    qvnc.cpp \
    qvncclient.cpp

HEADERS = \
    qvncintegration.h \
    qvncscreen.h \
    qvnc_p.h \
    qvncclient.h

OTHER_FILES += vnc.json

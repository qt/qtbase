TARGET = qvnc

PLUGIN_TYPE = platforms
PLUGIN_CLASS_NAME = QVncIntegrationPlugin
!equals(TARGET, $$QT_DEFAULT_QPA_PLUGIN): PLUGIN_EXTENDS = -
load(qt_plugin)

QT += core-private gui-private platformsupport-private network

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

CONFIG += qpa/genericunixfontdatabase

OTHER_FILES += vnc.json

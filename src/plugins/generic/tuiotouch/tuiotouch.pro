TARGET = qtuiotouchplugin

QT += \
    core-private \
    gui-private \
    network

SOURCES += \
    main.cpp \
    qoscbundle.cpp \
    qoscmessage.cpp \
    qtuiohandler.cpp

HEADERS += \
    qoscbundle_p.h \
    qoscmessage_p.h \
    qtuiohandler_p.h \
    qtuiocursor_p.h \
    qtuiotoken_p.h

OTHER_FILES += \
    tuiotouch.json

DEFINES += QT_NO_FOREACH
PLUGIN_TYPE = generic
PLUGIN_EXTENDS = -
PLUGIN_CLASS_NAME = QTuioTouchPlugin
load(qt_plugin)

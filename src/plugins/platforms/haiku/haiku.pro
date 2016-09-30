TARGET = qhaiku

QT += core-private gui-private eventdistpatcher_support-private

SOURCES = \
    main.cpp \
    qhaikuapplication.cpp \
    qhaikubuffer.cpp \
    qhaikuclipboard.cpp \
    qhaikucursor.cpp \
    qhaikuintegration.cpp \
    qhaikukeymapper.cpp \
    qhaikurasterbackingstore.cpp \
    qhaikurasterwindow.cpp \
    qhaikuscreen.cpp \
    qhaikuservices.cpp \
    qhaikuutils.cpp \
    qhaikuwindow.cpp

HEADERS = \
    main.h \
    qhaikuapplication.h \
    qhaikubuffer.h \
    qhaikuclipboard.h \
    qhaikucursor.h \
    qhaikuintegration.h \
    qhaikukeymapper.h \
    qhaikurasterbackingstore.h \
    qhaikurasterwindow.h \
    qhaikuscreen.h \
    qhaikuservices.h \
    qhaikuutils.h \
    qhaikuwindow.h

LIBS += -lbe

OTHER_FILES += haiku.json

PLUGIN_TYPE = platforms
PLUGIN_CLASS_NAME = QHaikuIntegrationPlugin
load(qt_plugin)

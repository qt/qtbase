TARGET = qwindows

PLUGIN_TYPE = platforms
PLUGIN_CLASS_NAME = QWindowsIntegrationPlugin
load(qt_plugin)

QT *= core-private
QT *= gui-private
QT *= platformsupport-private

!wince:LIBS *= -lgdi32

include(windows.pri)

SOURCES +=  \
    main.cpp \
    qwindowsbackingstore.cpp \
    qwindowsgdiintegration.cpp \
    qwindowsgdinativeinterface.cpp

HEADERS +=  \
    qwindowsbackingstore.h \
    qwindowsgdiintegration.h \
    qwindowsgdinativeinterface.h

OTHER_FILES += windows.json

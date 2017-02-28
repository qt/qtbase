TARGET = qwindowsvistastyle

QT += widgets-private

SOURCES += main.cpp

HEADERS += qwindowsvistastyle_p.h qwindowsvistastyle_p_p.h
SOURCES += qwindowsvistastyle.cpp

HEADERS += qwindowsxpstyle_p.h qwindowsxpstyle_p_p.h
SOURCES += qwindowsxpstyle.cpp

LIBS_PRIVATE += -lgdi32 -luser32

# DEFINES/LIBS needed for qwizard_win.cpp and the styles
include(../../../widgets/kernel/win.pri)

DISTFILES += windowsvistastyle.json

PLUGIN_TYPE = styles
PLUGIN_CLASS_NAME = QWindowsVistaStylePlugin
load(qt_plugin)

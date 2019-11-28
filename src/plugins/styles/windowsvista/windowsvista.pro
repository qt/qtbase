TARGET = qwindowsvistastyle

QT += widgets-private

SOURCES += main.cpp

HEADERS += qwindowsvistastyle_p.h qwindowsvistastyle_p_p.h
SOURCES += qwindowsvistastyle.cpp

HEADERS += qwindowsxpstyle_p.h qwindowsxpstyle_p_p.h
SOURCES += qwindowsxpstyle.cpp

QMAKE_USE_PRIVATE += user32 gdi32
LIBS_PRIVATE *= -luxtheme

DISTFILES += windowsvistastyle.json

PLUGIN_TYPE = styles
PLUGIN_CLASS_NAME = QWindowsVistaStylePlugin
load(qt_plugin)

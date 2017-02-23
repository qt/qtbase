TARGET = qconsole

QT += core-private gui-private platformsupport-private

SOURCES =   main.cpp \
            qconsoleintegration.cpp \
            qconsolebackingstore.cpp
win32{
SOURCES +=  qwindowsfontdatabase.cpp \
			qwindowsfontengine.cpp \
			qwindowsnativeimage.cpp \
			qwindowsfontenginedirectwrite.cpp
}
			
HEADERS =   qconsoleintegration.h \
            qconsolebackingstore.h

win32{
HEADERS += 	qwindowsfontdatabase.h \
			qtwindows_additional.h \
			qwindowsfontengine.h \
			qwindowsnativeimage.h \
			qtwindowsglobal.h \
			qwindowsfontenginedirectwrite.h
}

OTHER_FILES += console.json

CONFIG += qpa/genericunixfontdatabase

PLUGIN_TYPE = platforms
PLUGIN_CLASS_NAME = QConsoleIntegrationPlugin
!equals(TARGET, $$QT_DEFAULT_QPA_PLUGIN): PLUGIN_EXTENDS = -
load(qt_plugin)

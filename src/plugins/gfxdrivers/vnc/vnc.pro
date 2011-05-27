TARGET = qgfxvnc
load(qt_plugin)

DEFINES	+= QT_QWS_VNC

DESTDIR = $$QT.gui.plugins/gfxdrivers

HEADERS = \
	qscreenvnc_qws.h \
	qscreenvnc_p.h

SOURCES = main.cpp \
	qscreenvnc_qws.cpp

target.path += $$[QT_INSTALL_PLUGINS]/gfxdrivers
INSTALLS += target

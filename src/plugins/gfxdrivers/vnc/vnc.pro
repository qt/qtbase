TARGET = qgfxvnc
include(../../qpluginbase.pri)

DEFINES	+= QT_QWS_VNC

QTDIR_build:DESTDIR = $$QT_BUILD_TREE/plugins/gfxdrivers

HEADERS = \
	qscreenvnc_qws.h \
	qscreenvnc_p.h

SOURCES = main.cpp \
	qscreenvnc_qws.cpp

target.path += $$[QT_INSTALL_PLUGINS]/gfxdrivers
INSTALLS += target

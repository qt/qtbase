TARGET   = qimsw-multi
include(../../qpluginbase.pri)
CONFIG      += warn_on

DESTDIR = $$QT.gui.plugins/inputmethods

HEADERS += qmultiinputcontext.h \
           qmultiinputcontextplugin.h
SOURCES += qmultiinputcontext.cpp \
           qmultiinputcontextplugin.cpp

target.path += $$[QT_INSTALL_PLUGINS]/inputmethods
INSTALLS    += target

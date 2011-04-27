TARGET   = qimsw-multi
include(../../qpluginbase.pri)
CONFIG      += warn_on

QTDIR_build:DESTDIR = $$QT_BUILD_TREE/plugins/inputmethods

HEADERS += qmultiinputcontext.h \
           qmultiinputcontextplugin.h
SOURCES += qmultiinputcontext.cpp \
           qmultiinputcontextplugin.cpp

target.path += $$[QT_INSTALL_PLUGINS]/inputmethods
INSTALLS    += target

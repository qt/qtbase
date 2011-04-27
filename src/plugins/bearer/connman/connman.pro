TARGET = qconnmanbearer
include(../../qpluginbase.pri)

QT = core network dbus

HEADERS += qconnmanservice_linux_p.h \
           qofonoservice_linux_p.h \
           qconnmanengine.h \
           ../qnetworksession_impl.h \
           ../qbearerengine_impl.h

SOURCES += main.cpp \
           qconnmanservice_linux.cpp \
           qofonoservice_linux.cpp \
           qconnmanengine.cpp \
           ../qnetworksession_impl.cpp

QTDIR_build:DESTDIR = $$QT_BUILD_TREE/plugins/bearer
target.path += $$[QT_INSTALL_PLUGINS]/bearer
INSTALLS += target


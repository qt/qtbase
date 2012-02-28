TARGET = qnmbearer
load(qt_plugin)

QT = core network-private dbus

HEADERS += qnmdbushelper.h \
           qnetworkmanagerservice.h \
           qnetworkmanagerengine.h \
           ../qnetworksession_impl.h \
           ../qbearerengine_impl.h

SOURCES += main.cpp \
           qnmdbushelper.cpp \
           qnetworkmanagerservice.cpp \
           qnetworkmanagerengine.cpp \
           ../qnetworksession_impl.cpp

OTHER_FILES += networkmanager.json

DESTDIR = $$QT.network.plugins/bearer
target.path += $$[QT_INSTALL_PLUGINS]/bearer
INSTALLS += target

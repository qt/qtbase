TARGET = qgenericbearer
load(qt_plugin)

QT = core-private network-private

HEADERS += qgenericengine.h \
           ../qnetworksession_impl.h \
           ../qbearerengine_impl.h \
           ../platformdefs_win.h
SOURCES += qgenericengine.cpp \
           ../qnetworksession_impl.cpp \
           main.cpp

DESTDIR = $$QT.network.plugins/bearer
target.path += $$[QT_INSTALL_PLUGINS]/bearer
INSTALLS += target

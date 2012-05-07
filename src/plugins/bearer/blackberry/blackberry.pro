TARGET = qbbbearer
load(qt_plugin)

QT = core-private network-private

# Uncomment this to enable debugging output for the plugin
#DEFINES += QBBENGINE_DEBUG

HEADERS += qbbengine.h \
           ../qnetworksession_impl.h \
           ../qbearerengine_impl.h

SOURCES += qbbengine.cpp \
           ../qnetworksession_impl.cpp \
           main.cpp

OTHER_FILES += blackberry.json

DESTDIR = $$QT.network.plugins/bearer
target.path += $$[QT_INSTALL_PLUGINS]/bearer
INSTALLS += target

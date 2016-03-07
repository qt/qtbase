TARGET = qnlabearer

QT = core core-private network network-private

LIBS += -lws2_32

HEADERS += qnlaengine.h \
           ../platformdefs_win.h \
           ../qnetworksession_impl.h \
           ../qbearerengine_impl.h

SOURCES += main.cpp \
           qnlaengine.cpp \
           ../qnetworksession_impl.cpp

OTHER_FILES += nla.json

PLUGIN_TYPE = bearer
PLUGIN_CLASS_NAME = QNlaEnginePlugin
load(qt_plugin)

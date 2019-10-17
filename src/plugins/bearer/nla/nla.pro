TARGET = qnlabearer

QT = core core-private network network-private

QMAKE_USE_PRIVATE += ws2_32

HEADERS += qnlaengine.h \
           ../platformdefs_win.h

SOURCES += main.cpp \
           qnlaengine.cpp

OTHER_FILES += nla.json

PLUGIN_TYPE = bearer
PLUGIN_CLASS_NAME = QNlaEnginePlugin
load(qt_plugin)

TARGET = composeplatforminputcontextplugin

QT += core-private gui-private

SOURCES += $$PWD/qcomposeplatforminputcontextmain.cpp \
           $$PWD/qcomposeplatforminputcontext.cpp

HEADERS += $$PWD/qcomposeplatforminputcontext.h

QMAKE_USE_PRIVATE += xkbcommon

include($$OUT_PWD/../../../gui/qtgui-config.pri)

OTHER_FILES += $$PWD/compose.json

PLUGIN_TYPE = platforminputcontexts
PLUGIN_EXTENDS = -
PLUGIN_CLASS_NAME = QComposePlatformInputContextPlugin
load(qt_plugin)

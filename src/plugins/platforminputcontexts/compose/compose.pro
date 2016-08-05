TARGET = composeplatforminputcontextplugin

QT += core-private gui-private

DEFINES += X11_PREFIX='\\"$$QMAKE_X11_PREFIX\\"'

SOURCES += $$PWD/qcomposeplatforminputcontextmain.cpp \
           $$PWD/qcomposeplatforminputcontext.cpp \
           $$PWD/generator/qtablegenerator.cpp \

HEADERS += $$PWD/qcomposeplatforminputcontext.h \
           $$PWD/generator/qtablegenerator.h \

# libxkbcommon
!qtConfig(xkbcommon-system) {
    include(../../../3rdparty/xkbcommon.pri)
} else {
    QMAKE_USE += xkbcommon
}

OTHER_FILES += $$PWD/compose.json

PLUGIN_TYPE = platforminputcontexts
PLUGIN_EXTENDS = -
PLUGIN_CLASS_NAME = QComposePlatformInputContextPlugin
load(qt_plugin)

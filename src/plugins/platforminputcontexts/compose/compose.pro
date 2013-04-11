TARGET = composeplatforminputcontextplugin

PLUGIN_TYPE = platforminputcontexts
PLUGIN_CLASS_NAME = QComposePlatformInputContextPlugin
load(qt_plugin)

QT += gui-private

LIBS += $$QMAKE_LIBS_XKBCOMMON
QMAKE_CXXFLAGS += $$QMAKE_CFLAGS_XKBCOMMON

SOURCES += $$PWD/main.cpp \
           $$PWD/qcomposeplatforminputcontext.cpp \
           $$PWD/generator/qtablegenerator.cpp \

HEADERS += $$PWD/qcomposeplatforminputcontext.h \
           $$PWD/generator/qtablegenerator.h \
           $$PWD/xkbcommon_workaround.h \

# libxkbcommon
contains(QT_CONFIG, xkbcommon-qt): {
    include(../../../3rdparty/xkbcommon.pri)
} else {
    LIBS += $$QMAKE_LIBS_XKBCOMMON
    QMAKE_CXXFLAGS += $$QMAKE_CFLAGS_XKBCOMMON
    LIBS += -lxkbcommon
}

OTHER_FILES += $$PWD/compose.json

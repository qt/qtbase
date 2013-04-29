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

# libxkbcommon
contains(QT_CONFIG, xkbcommon-qt): {
    include(../../../3rdparty/xkbcommon.pri)
} else {
    LIBS += $$QMAKE_LIBS_XKBCOMMON
    QMAKE_CXXFLAGS += $$QMAKE_CFLAGS_XKBCOMMON
    equals(QMAKE_VERSION_XKBCOMMON, "0.2.0") {
        DEFINES += XKBCOMMON_0_2_0
        INCLUDEPATH += ../../../3rdparty/xkbcommon/xkbcommon/
    }
}

OTHER_FILES += $$PWD/compose.json

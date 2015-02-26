TARGET = tizenscimplatforminputcontextplugin

PLUGIN_TYPE = platforminputcontexts
PLUGIN_CLASS_NAME = QTizenScimPlatformInputContextPlugin
load(qt_plugin)

packagesExist(scim) {
    CONFIG += link_pkgconfig
    PKGCONFIG += scim
} else {
    INCLUDEPATH += $$[QT_SYSROOT]/usr/include/scim-1.0
}

LIBS += -lscim-1.0
QT += gui-private network

SOURCES += main.cpp \
           qtizenscimplatforminputcontext.cpp

HEADERS += qtizenscimplatforminputcontext.h

OTHER_FILES += $$PWD/compose.json

TARGET = ibusplatforminputcontextplugin
load(qt_plugin)

QTDIR_build:DESTDIR = $$QT_BUILD_TREE/plugins/platforminputcontexts

QT += dbus platformsupport-private
SOURCES += $$PWD/qibusplatforminputcontext.cpp \
           $$PWD/qibusproxy.cpp \
           $$PWD/qibusinputcontextproxy.cpp \
           $$PWD/qibustypes.cpp \
           $$PWD/main.cpp

HEADERS += $$PWD/qibusplatforminputcontext.h \
           $$PWD/qibusproxy.h \
           $$PWD/qibusinputcontextproxy.h \
           $$PWD/qibustypes.h

OTHER_FILES += $$PWD/ibus.json

target.path += $$[QT_INSTALL_PLUGINS]/platforminputcontexts
INSTALLS += target

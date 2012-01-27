TARGET = meegoplatforminputcontextplugin
load(qt_plugin)

QTDIR_build:DESTDIR = $$QT_BUILD_TREE/plugins/platforminputcontexts

QT += dbus platformsupport-private
SOURCES += $$PWD/qmeegoplatforminputcontext.cpp \
           $$PWD/serverproxy.cpp \
           $$PWD/serveraddressproxy.cpp \
           $$PWD/contextadaptor.cpp \
           $$PWD/main.cpp

HEADERS += $$PWD/qmeegoplatforminputcontext.h \
           $$PWD/serverproxy.h \
           $$PWD/serveraddressproxy.h \
           $$PWD/contextadaptor.h

target.path += $$[QT_INSTALL_PLUGINS]/platforminputcontexts
INSTALLS += target

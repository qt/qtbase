TARGET = qevdevkeyboardplugin
load(qt_plugin)

DESTDIR = $$QT.gui.plugins/generic
target.path = $$[QT_INSTALL_PLUGINS]/generic
INSTALLS += target

HEADERS = \
    qevdevkeyboard_defaultmap.h \
    qevdevkeyboardhandler.h \
    qevdevkeyboardmanager.h

QT += core-private platformsupport-private

SOURCES = main.cpp \
    qevdevkeyboardhandler.cpp \
    qevdevkeyboardmanager.cpp

OTHER_FILES += \
    evdevkeyboard.json

contains(QT_CONFIG, libudev) {
    LIBS += $$QMAKE_LIBS_LIBUDEV
}

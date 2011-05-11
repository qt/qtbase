TEMPLATE = subdirs
SUBDIRS = \
        corelib \
        gui \
        network \

contains(QT_CONFIG, opengl): SUBDIRS += opengl
contains(QT_CONFIG, dbus): SUBDIRS += dbus

check-trusted.CONFIG += recursive
QMAKE_EXTRA_TARGETS += check-trusted

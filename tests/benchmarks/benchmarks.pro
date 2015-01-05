TEMPLATE = subdirs
SUBDIRS = \
        corelib \
        sql \

# removed-by-refactor qtHaveModule(opengl): SUBDIRS += opengl
qtHaveModule(dbus): SUBDIRS += dbus
qtHaveModule(network): SUBDIRS += network
qtHaveModule(gui): SUBDIRS += gui

check-trusted.CONFIG += recursive
QMAKE_EXTRA_TARGETS += check-trusted

TEMPLATE = subdirs
SUBDIRS = \
        corelib \
        sql \

qtHaveModule(dbus): SUBDIRS += dbus
qtHaveModule(gui): SUBDIRS += gui
qtHaveModule(network): SUBDIRS += network
# removed-by-refactor qtHaveModule(opengl): SUBDIRS += opengl
qtHaveModule(testlib): SUBDIRS += testlib
qtHaveModule(widgets): SUBDIRS += widgets

check-trusted.CONFIG += recursive
QMAKE_EXTRA_TARGETS += check-trusted

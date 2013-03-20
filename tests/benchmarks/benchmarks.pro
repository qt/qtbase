TEMPLATE = subdirs
SUBDIRS = \
        corelib \
        gui \
        sql \

# removed-by-refactor qtHaveModule(opengl): SUBDIRS += opengl
qtHaveModule(dbus): SUBDIRS += dbus
qtHaveModule(network): SUBDIRS += network

check-trusted.CONFIG += recursive
QMAKE_EXTRA_TARGETS += check-trusted

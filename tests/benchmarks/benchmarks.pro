TEMPLATE = subdirs
SUBDIRS = \
        corelib \
        gui \
        network \
        sql \

# removed-by-refactor qtHaveModule(opengl): SUBDIRS += opengl
qtHaveModule(dbus): SUBDIRS += dbus

check-trusted.CONFIG += recursive
QMAKE_EXTRA_TARGETS += check-trusted

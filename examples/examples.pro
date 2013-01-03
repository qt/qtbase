TEMPLATE      = subdirs
CONFIG += no_docs_target

SUBDIRS       = \
                gui \
                network \
                threads \
                xml \
                qpa

qtHaveModule(widgets) {
    SUBDIRS += widgets \
               ipc \
               sql \
               tools \
               touch \
               gestures
}

wince*|embedded|x11:qtHaveModule(gui): SUBDIRS += embedded

contains(QT_BUILD_PARTS, tools):qtHaveModule(gui):qtHaveModule(widgets): SUBDIRS += qtestlib
qtHaveModule(opengl):qtHaveModule(widgets): SUBDIRS += opengl
qtHaveModule(dbus): SUBDIRS += dbus
qtHaveModule(concurrent): SUBDIRS += qtconcurrent

aggregate.files = aggregate/examples.pro
aggregate.path = $$[QT_INSTALL_EXAMPLES]
readme.files = README
readme.path = $$[QT_INSTALL_EXAMPLES]
INSTALLS += aggregate readme

TEMPLATE      = subdirs
CONFIG += no_docs_target

SUBDIRS       = \
                gui \
                network \
                threads \
                xml \
                qpa

!contains(QT_CONFIG, no-widgets) {
    SUBDIRS += widgets \
               ipc \
               linguist \
               sql \
               tools \
               touch \
               gestures
}

wince*|embedded|x11:!contains(QT_CONFIG, no-gui): SUBDIRS += embedded

contains(QT_BUILD_PARTS, tools):!contains(QT_CONFIG, no-gui):!contains(QT_CONFIG, no-widgets):SUBDIRS += qtestlib
contains(QT_CONFIG, opengl):!contains(QT_CONFIG, no-widgets):SUBDIRS += opengl
contains(QT_CONFIG, dbus): SUBDIRS += dbus
contains(QT_CONFIG, concurrent): SUBDIRS += qtconcurrent
contains(DEFINES, QT_NO_TRANSLATION): SUBDIRS -= linguist

aggregate.files = aggregate/examples.pro
aggregate.path = $$[QT_INSTALL_EXAMPLES]
readme.files = README
readme.path = $$[QT_INSTALL_EXAMPLES]
INSTALLS += aggregate readme

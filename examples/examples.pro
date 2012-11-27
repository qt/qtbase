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

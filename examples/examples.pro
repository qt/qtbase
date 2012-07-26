TEMPLATE      = subdirs
CONFIG += no_docs_target

SUBDIRS       = \
                network \
                statemachine \
                threads \
                xml \
                qpa

!contains(QT_CONFIG, no-widgets) {
    SUBDIRS  += \
                animation \
                desktop \
                dialogs \
                draganddrop \
                effects \
                graphicsview \
                ipc \
                layouts \
                linguist \
                mainwindows \
                painting \
                richtext \
                scroller \
                sql \
                tools \
                tutorials \
                touch \
                gestures

    !contains(QT_CONFIG, no-widgets) {
        SUBDIRS +=  widgets \
                    itemviews
    }
}
wince*|embedded|x11:!contains(QT_CONFIG, no-gui): SUBDIRS += embedded

contains(QT_BUILD_PARTS, tools):!contains(QT_CONFIG, no-gui):!contains(QT_CONFIG, no-widgets):SUBDIRS += qtestlib
contains(QT_CONFIG, opengl):!contains(QT_CONFIG, no-widgets):SUBDIRS += opengl
contains(QT_CONFIG, dbus): SUBDIRS += dbus
contains(DEFINES, QT_NO_CURSOR): SUBDIRS -= mainwindows
contains(QT_CONFIG, concurrent): SUBDIRS += qtconcurrent

# install
sources.files = README *.pro
sources.path = $$[QT_INSTALL_EXAMPLES]
INSTALLS += sources

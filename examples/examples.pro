Qt += widgets

TEMPLATE      = subdirs
SUBDIRS       = \
                network \
                statemachine \
                threads \
                xml

!contains(QT_CONFIG, no-gui) {
    SUBDIRS  += \
                animation \
                desktop \
                dialogs \
                draganddrop \
                effects \
                graphicsview \
                ipc \
                itemviews \
                layouts \
                linguist \
                mainwindows \
                painting \
                richtext \
                scroller \
                sql \
                tools \
                tutorials \
                widgets \
                uitools \
                touch \
                gestures
}

symbian: SUBDIRS = \
                graphicsview \
                itemviews \
                network \
                painting \
                widgets \
                draganddrop \
                mainwindows \
                sql \
                uitools \
                animation \
                gestures \
                xml

qpa:SUBDIRS += qpa
!contains(QT_CONFIG, no-gui):contains(QT_CONFIG, multimedia) {
    SUBDIRS += multimedia
}

wince*|symbian|embedded|x11:!contains(QT_CONFIG, no-gui): SUBDIRS += embedded

embedded:SUBDIRS += qws
contains(QT_BUILD_PARTS, tools):!contains(QT_CONFIG, no-gui):SUBDIRS += qtestlib
contains(QT_CONFIG, opengl): SUBDIRS += opengl
contains(QT_CONFIG, dbus): SUBDIRS += dbus
contains(DEFINES, QT_NO_CURSOR): SUBDIRS -= mainwindows
contains(QT_CONFIG, concurrent): SUBDIRS += qtconcurrent

# install
sources.files = README *.pro
sources.path = $$[QT_INSTALL_EXAMPLES]
INSTALLS += sources

symbian: CONFIG += qt_example
QT += widgets
maemo5: CONFIG += qt_example

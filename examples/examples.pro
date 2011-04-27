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

contains(QT_CONFIG, webkit):SUBDIRS += webkit

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

!contains(QT_CONFIG, no-gui):contains(QT_CONFIG, multimedia) {
    SUBDIRS += multimedia
}

contains(QT_CONFIG, script): SUBDIRS += script

contains(QT_CONFIG, phonon):!static: SUBDIRS += phonon
embedded:SUBDIRS += qws
!wince*:!symbian: {
    !contains(QT_EDITION, Console):!contains(QT_CONFIG, no-gui):contains(QT_BUILD_PARTS, tools):SUBDIRS += designer
    contains(QT_BUILD_PARTS, tools):!contains(QT_CONFIG, no-gui):SUBDIRS += qtestlib help
} else {
    contains(QT_BUILD_PARTS, tools):!contains(QT_CONFIG, no-gui):SUBDIRS += qtestlib
}
contains(QT_CONFIG, opengl): SUBDIRS += opengl
contains(QT_CONFIG, openvg): SUBDIRS += openvg
contains(QT_CONFIG, dbus): SUBDIRS += dbus
contains(QT_CONFIG, declarative): SUBDIRS += declarative
win32: SUBDIRS += activeqt
contains(QT_CONFIG, xmlpatterns):!contains(QT_CONFIG, no-gui): SUBDIRS += xmlpatterns
contains(DEFINES, QT_NO_CURSOR): SUBDIRS -= mainwindows
contains(QT_CONFIG, concurrent): SUBDIRS += qtconcurrent

# install
sources.files = README *.pro
sources.path = $$[QT_INSTALL_EXAMPLES]
INSTALLS += sources

symbian: include($$QT_SOURCE_TREE/examples/symbianpkgrules.pri)

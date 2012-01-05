TEMPLATE = subdirs

SUBDIRS += \
    corelib \
    dbus \
    gui \
    network \
    opengl \
    sql \
    testlib \
    tools \
    xml \
    other \
    widgets \

cross_compile:                              SUBDIRS -= tools
!contains(QT_CONFIG, opengl):               SUBDIRS -= opengl
!unix|embedded|!contains(QT_CONFIG, dbus):  SUBDIRS -= dbus

# disable 'make check' on Mac OS X for the following subdirs for the time being
mac {
    network.CONFIG += no_check_target
    widgets.CONFIG += no_check_target
}

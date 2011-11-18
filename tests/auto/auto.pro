TEMPLATE = subdirs

SUBDIRS += \
    corelib \
    dbus \
    gui \
    integrationtests \
    network \
    opengl \
    sql \
    testlib \
    tools \
    v8 \
    xml \
    other \
    widgets \

cross_compile:                              SUBDIRS -= tools
!contains(QT_CONFIG, opengl):               SUBDIRS -= opengl
!unix|embedded|!contains(QT_CONFIG, dbus):  SUBDIRS -= dbus
!contains(QT_CONFIG, v8):                   SUBDIRS -= v8

# disable 'make check' on Mac OS X for the following subdirs for the time being
mac {
    gui.CONFIG += no_check_target
    network.CONFIG += no_check_target
    opengl.CONFIG += no_check_target
    sql.CONFIG += no_check_target
    v8.CONFIG += no_check_target
    widgets.CONFIG += no_check_target
}

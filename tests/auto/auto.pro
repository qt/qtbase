TEMPLATE = subdirs

SUBDIRS += \
    corelib.pro \
    dbus \
    host.pro \
    gui.pro \
    integrationtests \
    network.pro \
    opengl \
    sql \
    testlib \
    tools \
    v8.pro \
    xml \
    other.pro \
    widgets \

cross_compile:                              SUBDIRS -= host.pro
cross_compile:                              SUBDIRS -= tools
!contains(QT_CONFIG, opengl):               SUBDIRS -= opengl
!unix|embedded|!contains(QT_CONFIG, dbus):  SUBDIRS -= dbus
!contains(QT_CONFIG, v8):                   SUBDIRS -= v8.pro

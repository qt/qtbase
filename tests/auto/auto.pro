TEMPLATE = subdirs

SUBDIRS += \
    corelib.pro \
    dbus \
    host.pro \
    gui.pro \
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

cross_compile:                              SUBDIRS -= host.pro
cross_compile:                              SUBDIRS -= tools
!contains(QT_CONFIG, opengl):               SUBDIRS -= opengl
!unix|embedded|!contains(QT_CONFIG, dbus):  SUBDIRS -= dbus
!contains(QT_CONFIG, v8):                   SUBDIRS -= v8.pro

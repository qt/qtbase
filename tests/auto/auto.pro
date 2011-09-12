TEMPLATE = subdirs

SUBDIRS += \
    corelib.pro \
    gui.pro \
    network.pro \
    sql \
    xml \
    testlib \
    other \

!cross_compile:                             SUBDIRS += host.pro
!cross_compile:                             SUBDIRS += tools
contains(QT_CONFIG, opengl):                SUBDIRS += opengl
unix:!embedded:contains(QT_CONFIG, dbus):   SUBDIRS += dbus
contains(QT_CONFIG, v8):                    SUBDIRS += v8.pro

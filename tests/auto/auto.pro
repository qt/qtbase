TEMPLATE = subdirs

SUBDIRS += \
    corelib.pro \
    gui.pro \
    network.pro \
    sql.pro \
    xml.pro \
    other.pro

!cross_compile:                             SUBDIRS += host.pro
contains(QT_CONFIG, opengl):                SUBDIRS += opengl.pro
unix:!embedded:contains(QT_CONFIG, dbus):   SUBDIRS += dbus
contains(QT_CONFIG, v8):                    SUBDIRS += v8.pro

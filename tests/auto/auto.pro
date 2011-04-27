TEMPLATE = subdirs

SUBDIRS += \
    corelib.pro \
    gui.pro \
    network.pro \
    sql.pro \
    xml.pro \
    other.pro

!cross_compile:                             SUBDIRS += host.pro
contains(QT_CONFIG, qt3support):!wince*:    SUBDIRS += qt3support.pro
contains(QT_CONFIG, opengl):                SUBDIRS += opengl.pro
contains(QT_CONFIG, xmlpatterns):           SUBDIRS += xmlpatterns.pro
unix:!embedded:contains(QT_CONFIG, dbus):   SUBDIRS += dbus.pro
contains(QT_CONFIG, script):                SUBDIRS += script.pro
contains(QT_CONFIG, webkit):                SUBDIRS += webkit.pro
contains(QT_CONFIG, multimedia):            SUBDIRS += multimedia.pro
contains(QT_CONFIG, phonon):                SUBDIRS += phonon.pro
contains(QT_CONFIG, svg):                   SUBDIRS += svg.pro
contains(QT_CONFIG, declarative):           SUBDIRS += declarative.pro
!symbian                                    SUBDIRS += help.pro


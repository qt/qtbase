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
    concurrent \
    other \
    widgets \
    printsupport \
    cmake

wince*|contains(DEFINES, QT_NO_PRINTER):    SUBDIRS -= printsupport
cross_compile:                              SUBDIRS -= tools
isEmpty(QT.opengl.name):                    SUBDIRS -= opengl
!unix|embedded|!contains(QT_CONFIG, dbus):  SUBDIRS -= dbus
contains(QT_CONFIG, no-widgets):            SUBDIRS -= widgets printsupport
!contains(QT_CONFIG, concurrent):           SUBDIRS -= concurrent

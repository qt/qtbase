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
    cmake \
    installed_cmake

installed_cmake.depends = cmake

wince*:                                     SUBDIRS -= printsupport
cross_compile:                              SUBDIRS -= tools
!qtHaveModule(opengl):                      SUBDIRS -= opengl
!unix|embedded|!qtHaveModule(dbus):         SUBDIRS -= dbus
!qtHaveModule(widgets):                     SUBDIRS -= widgets
!qtHaveModule(printsupport):                SUBDIRS -= printsupport
!qtHaveModule(concurrent):                  SUBDIRS -= concurrent
!qtHaveModule(network):                     SUBDIRS -= network

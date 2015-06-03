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

ios: SUBDIRS  = corelib gui

wince:                                      SUBDIRS -= printsupport
cross_compile:                              SUBDIRS -= tools
!qtHaveModule(opengl):                      SUBDIRS -= opengl
!qtHaveModule(gui):                         SUBDIRS -= gui cmake
!qtHaveModule(widgets):                     SUBDIRS -= widgets
!qtHaveModule(printsupport):                SUBDIRS -= printsupport
!qtHaveModule(concurrent):                  SUBDIRS -= concurrent
!qtHaveModule(network):                     SUBDIRS -= network

# Disable the QtDBus tests if we can't connect to the session bus
qtHaveModule(dbus) {
    !system("dbus-send --session --type=signal / local.AutotestCheck.Hello"): {
        warning("QtDBus is enabled but session bus is not available. Please check the installation.")
        SUBDIRS -= dbus
    }
} else {
    SUBDIRS -= dbus
}

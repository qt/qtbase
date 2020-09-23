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

uikit: SUBDIRS  = corelib gui

cross_compile:                              SUBDIRS -= tools cmake installed_cmake
else:!qtConfig(process):                    SUBDIRS -= tools
winrt|!qtHaveModule(opengl):                SUBDIRS -= opengl
!qtHaveModule(gui):                         SUBDIRS -= gui
!qtHaveModule(widgets):                     SUBDIRS -= widgets
!qtHaveModule(printsupport):                SUBDIRS -= printsupport
!qtHaveModule(concurrent):                  SUBDIRS -= concurrent
winrt|!qtHaveModule(network):               SUBDIRS -= network
!qtHaveModule(dbus):                        SUBDIRS -= dbus
!qtHaveModule(xml):                         SUBDIRS -= xml
!qtHaveModule(sql):                         SUBDIRS -= sql

# Disable the QtDBus tests if we can't connect to the session bus
!cross_compile:qtHaveModule(dbus) {
    !system("dbus-send --session --type=signal / local.AutotestCheck.Hello >$$QMAKE_SYSTEM_NULL_DEVICE 2>&1") {
        qtConfig(dbus-linked): {
            warning("QtDBus is enabled but session bus is not available. QtDBus tests will fail.")
        } else {
            warning("QtDBus is enabled with runtime support, but session bus is not available. Skipping QtDBus tests.")
            SUBDIRS -= dbus
        }
    }
}

# QTBUG-63915
boot2qt: {
    SUBDIRS -= dbus
}

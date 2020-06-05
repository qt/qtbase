TEMPLATE = subdirs

uikit {
    SUBDIRS = corelib
    qtHaveModule(gui): SUBDIRS += gui
    return()
}

# Order by dependency [*], then alphabetic. [*] If bugs in part A of
# our source would break tests of part B, then test A before B.
SUBDIRS += testlib
qtConfig(process):!cross_compile: SUBDIRS += tools
SUBDIRS += corelib
!cross_compile: SUBDIRS += cmake
qtHaveModule(concurrent): SUBDIRS += concurrent
# QTBUG-63915: boot2qt fails dbus
qtHaveModule(dbus):!cross_compile:!boot2qt {
    # Disable the QtDBus tests if we can't connect to the session bus
    system("dbus-send --session --type=signal / local.AutotestCheck.Hello >$$QMAKE_SYSTEM_NULL_DEVICE 2>&1") {
        SUBDIRS += dbus
    } else {
        qtConfig(dbus-linked): \
            error("QtDBus is enabled but session bus is not available. Please check the installation.")
        else: \
            warning("QtDBus is enabled with runtime support, but session bus is not available. Skipping QtDBus tests.")
    }
}
qtHaveModule(gui): SUBDIRS += gui
qtHaveModule(network): SUBDIRS += network
qtHaveModule(opengl): SUBDIRS += opengl
qtHaveModule(printsupport): SUBDIRS += printsupport
qtHaveModule(sql): SUBDIRS += sql
qtHaveModule(widgets): SUBDIRS += widgets
qtHaveModule(xml): SUBDIRS += xml
!cross_compile: SUBDIRS += installed_cmake
SUBDIRS += other

installed_cmake.depends = cmake

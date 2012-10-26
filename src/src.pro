TEMPLATE = subdirs

src_tools.subdir = $$PWD/tools
src_tools.target = sub-tools

src_winmain.subdir = $$PWD/winmain
src_winmain.target = sub-winmain
src_winmain.depends = sub-corelib  # just for the module .pri file

src_corelib.subdir = $$PWD/corelib
src_corelib.target = sub-corelib
src_corelib.depends = src_tools

src_xml.subdir = $$PWD/xml
src_xml.target = sub-xml
src_xml.depends = src_corelib

src_dbus.subdir = $$PWD/dbus
src_dbus.target = sub-dbus
src_dbus.depends = src_corelib

src_concurrent.subdir = $$PWD/concurrent
src_concurrent.target = sub-concurrent
src_concurrent.depends = src_corelib

src_sql.subdir = $$PWD/sql
src_sql.target = sub-sql
src_sql.depends = src_corelib

src_network.subdir = $$PWD/network
src_network.target = sub-network
src_network.depends = src_corelib

src_testlib.subdir = $$PWD/testlib
src_testlib.target = sub-testlib
src_testlib.depends = src_corelib   # src_gui & src_widgets are not build-depends

src_angle.subdir = $$PWD/angle
src_angle.target = src_angle

src_gui.subdir = $$PWD/gui
src_gui.target = sub-gui
src_gui.depends = src_corelib

src_platformsupport.subdir = $$PWD/platformsupport
src_platformsupport.target = sub-platformsupport
src_platformsupport.depends = src_corelib src_gui src_network

src_widgets.subdir = $$PWD/widgets
src_widgets.target = sub-widgets
src_widgets.depends = src_corelib src_gui

src_opengl.subdir = $$PWD/opengl
src_opengl.target = sub-opengl
src_opengl.depends = src_gui src_widgets

src_printsupport.subdir = $$PWD/printsupport
src_printsupport.target = sub-printsupport
src_printsupport.depends = src_corelib src_gui src_widgets

src_plugins.subdir = $$PWD/plugins
src_plugins.target = sub-plugins
src_plugins.depends = src_sql src_xml src_network

# this order is important
SUBDIRS += src_tools src_corelib
win32:SUBDIRS += src_winmain
SUBDIRS += src_network src_sql src_xml src_testlib
contains(QT_CONFIG, dbus) {
    SUBDIRS += src_dbus
    src_plugins.depends += src_dbus
}
contains(QT_CONFIG, concurrent):SUBDIRS += src_concurrent
!contains(QT_CONFIG, no-gui) {
    win32:contains(QT_CONFIG, angle) {
        SUBDIRS += src_angle
        src_gui.depends += src_angle
    }
    SUBDIRS += src_gui src_platformsupport
    src_plugins.depends += src_gui src_platformsupport
    !contains(QT_CONFIG, no-widgets) {
        SUBDIRS += src_widgets
        src_plugins.depends += src_widgets
        contains(QT_CONFIG, opengl(es1|es2)?) {
            SUBDIRS += src_opengl
            src_plugins.depends += src_opengl
        }
        !wince* {
            SUBDIRS += src_printsupport
            src_plugins.depends += src_printsupport
        }
    }
}
SUBDIRS += src_plugins

nacl: SUBDIRS -= src_network src_testlib

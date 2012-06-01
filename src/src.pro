TEMPLATE = subdirs

# this order is important
win32:SRC_SUBDIRS += src_winmain
!wince*:SRC_SUBDIRS += src_tools
SRC_SUBDIRS += src_corelib
SRC_SUBDIRS += src_network src_sql src_gui src_xml src_testlib src_platformsupport src_widgets
!wince*:SRC_SUBDIRS += src_printsupport
nacl: SRC_SUBDIRS -= src_network src_testlib
contains(QT_CONFIG, dbus):SRC_SUBDIRS += src_dbus
contains(QT_CONFIG, concurrent):SRC_SUBDIRS += src_concurrent

contains(QT_CONFIG, no-gui): SRC_SUBDIRS -= src_gui

contains(QT_CONFIG, opengl)|contains(QT_CONFIG, opengles1)|contains(QT_CONFIG, opengles2): SRC_SUBDIRS += src_opengl
SRC_SUBDIRS += src_plugins

src_tools.subdir = $$PWD/tools
src_tools.target = sub-tools
src_winmain.subdir = $$PWD/winmain
src_winmain.target = sub-winmain
src_corelib.subdir = $$PWD/corelib
src_corelib.target = sub-corelib
src_xml.subdir = $$PWD/xml
src_xml.target = sub-xml
src_dbus.subdir = $$PWD/dbus
src_dbus.target = sub-dbus
src_gui.subdir = $$PWD/gui
src_gui.target = sub-gui
src_sql.subdir = $$PWD/sql
src_sql.target = sub-sql
src_network.subdir = $$PWD/network
src_network.target = sub-network
src_opengl.subdir = $$PWD/opengl
src_opengl.target = sub-opengl
src_plugins.subdir = $$PWD/plugins
src_plugins.target = sub-plugins
src_widgets.subdir = $$PWD/widgets
src_widgets.target = sub-widgets
!wince*: {
    src_printsupport.subdir = $$PWD/printsupport
    src_printsupport.target = sub-printsupport
}
src_testlib.subdir = $$PWD/testlib
src_testlib.target = sub-testlib
src_platformsupport.subdir = $$PWD/platformsupport
src_platformsupport.target = sub-platformsupport
src_concurrent.subdir = $$PWD/concurrent
src_concurrent.target = sub-concurrent


#CONFIG += ordered
!wince*:!ordered {
   src_corelib.depends = src_tools
   src_gui.depends = src_corelib
   src_printsupport.depends = src_corelib src_gui src_widgets
   src_platformsupport.depends = src_corelib src_gui src_network
   src_widgets.depends = src_corelib src_gui
   src_xml.depends = src_corelib
   src_concurrent.depends = src_corelib
   src_dbus.depends = src_corelib
   src_network.depends = src_corelib
   src_opengl.depends = src_gui src_widgets
   src_sql.depends = src_corelib
   src_testlib.depends = src_corelib src_gui src_widgets
   src_plugins.depends = src_gui src_sql src_xml src_platformsupport
}

contains(QT_CONFIG, no-widgets): SRC_SUBDIRS -= src_opengl src_widgets src_printsupport

SUBDIRS = $$SRC_SUBDIRS

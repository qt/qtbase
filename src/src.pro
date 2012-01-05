TEMPLATE = subdirs

# this order is important
unset(SRC_SUBDIRS)
win32:SRC_SUBDIRS += src_winmain
!wince*:include(tools/tools.pro)
SRC_SUBDIRS += src_corelib
SRC_SUBDIRS += src_network src_sql src_gui src_xml src_widgets src_printsupport src_testlib src_platformsupport
nacl: SRC_SUBDIRS -= src_network src_testlib
contains(QT_CONFIG, dbus):SRC_SUBDIRS += src_dbus

contains(QT_CONFIG, no-gui): SRC_SUBDIRS -= src_gui

contains(QT_CONFIG, opengl)|contains(QT_CONFIG, opengles1)|contains(QT_CONFIG, opengles2): SRC_SUBDIRS += src_opengl
SRC_SUBDIRS += src_plugins

src_winmain.subdir = $$QT_SOURCE_TREE/src/winmain
src_winmain.target = sub-winmain
src_corelib.subdir = $$QT_SOURCE_TREE/src/corelib
src_corelib.target = sub-corelib
src_xml.subdir = $$QT_SOURCE_TREE/src/xml
src_xml.target = sub-xml
src_dbus.subdir = $$QT_SOURCE_TREE/src/dbus
src_dbus.target = sub-dbus
src_gui.subdir = $$QT_SOURCE_TREE/src/gui
src_gui.target = sub-gui
src_sql.subdir = $$QT_SOURCE_TREE/src/sql
src_sql.target = sub-sql
src_network.subdir = $$QT_SOURCE_TREE/src/network
src_network.target = sub-network
src_opengl.subdir = $$QT_SOURCE_TREE/src/opengl
src_opengl.target = sub-opengl
src_plugins.subdir = $$QT_SOURCE_TREE/src/plugins
src_plugins.target = sub-plugins
src_widgets.subdir = $$QT_SOURCE_TREE/src/widgets
src_widgets.target = sub-widgets
src_printsupport.subdir = $$QT_SOURCE_TREE/src/printsupport
src_printsupport.target = sub-printsupport
src_testlib.subdir = $$QT_SOURCE_TREE/src/testlib
src_testlib.target = sub-testlib
src_platformsupport.subdir = $$QT_SOURCE_TREE/src/platformsupport
src_platformsupport.target = sub-platformsupport


#CONFIG += ordered
!wince*:!ordered {
   src_corelib.depends = src_tools_moc src_tools_rcc
   src_gui.depends = src_corelib
   src_printsupport.depends = src_corelib src_gui src_widgets
   src_platformsupport.depends = src_corelib src_gui src_network
   src_widgets.depends = src_corelib src_gui src_tools_uic
   src_xml.depends = src_corelib
   src_dbus.depends = src_corelib src_xml
   src_network.depends = src_corelib
   src_opengl.depends = src_gui src_widgets
   src_sql.depends = src_corelib
   src_testlib.depends = src_corelib src_gui src_widgets
   src_plugins.depends = src_gui src_sql src_xml
   qpa: src_plugins.depends = src_platformsupport
   contains(QT_CONFIG, opengl)|contains(QT_CONFIG, opengles1)|contains(QT_CONFIG, opengles2) {
      src_plugins.depends += src_opengl
      src_declarative.depends += src_opengl
      src_webkit.depends += src_opengl
   }
}


# This creates a sub-src rule
sub_src_target.CONFIG = recursive
sub_src_target.recurse = $$TOOLS_SUBDIRS $$SRC_SUBDIRS
sub_src_target.target = sub-src
sub_src_target.recurse_target =
QMAKE_EXTRA_TARGETS += sub_src_target

# This gives us a top level debug/release
for(subname, SRC_SUBDIRS) {
   subdir = $$subname
   !isEmpty($${subname}.subdir):subdir = $$eval($${subname}.subdir)
   subpro = $$subdir/$${basename(subdir)}.pro
   !exists($$subpro):next()
   subtarget = $$replace(subdir, [^A-Za-z0-9], _)
   reg_src = $$replace(QT_SOURCE_TREE, \\\\, \\\\)
   subdir = $$replace(subdir, $$reg_src, $$QT_BUILD_TREE)
   subdir = $$replace(subdir, /, $$QMAKE_DIR_SEP)
   subdir = $$replace(subdir, \\\\, $$QMAKE_DIR_SEP)
   SUB_TEMPLATE = $$list($$fromfile($$subpro, TEMPLATE))
   !isEqual(subname, src_tools_bootstrap):if(isEqual($$SUB_TEMPLATE, lib) | isEqual($$SUB_TEMPLATE, subdirs)):!separate_debug_info {
       #debug
       debug-$${subtarget}.depends = $${subdir}$${QMAKE_DIR_SEP}$(MAKEFILE) $$EXTRA_DEBUG_TARGETS
       debug-$${subtarget}.commands = (cd $$subdir && $(MAKE) -f $(MAKEFILE) debug)
       EXTRA_DEBUG_TARGETS += debug-$${subtarget}
       QMAKE_EXTRA_TARGETS += debug-$${subtarget}
       #release
       release-$${subtarget}.depends = $${subdir}$${QMAKE_DIR_SEP}$(MAKEFILE) $$EXTRA_RELEASE_TARGETS
       release-$${subtarget}.commands = (cd $$subdir && $(MAKE) -f $(MAKEFILE) release)
       EXTRA_RELEASE_TARGETS += release-$${subtarget}
       QMAKE_EXTRA_TARGETS += release-$${subtarget}
    } else { #do not have a real debug target/release
       #debug
       debug-$${subtarget}.depends = $${subdir}$${QMAKE_DIR_SEP}$(MAKEFILE) $$EXTRA_DEBUG_TARGETS
       debug-$${subtarget}.commands = (cd $$subdir && $(MAKE) -f $(MAKEFILE) first)
       EXTRA_DEBUG_TARGETS += debug-$${subtarget}
       QMAKE_EXTRA_TARGETS += debug-$${subtarget}
       #release
       release-$${subtarget}.depends = $${subdir}$${QMAKE_DIR_SEP}$(MAKEFILE) $$EXTRA_RELEASE_TARGETS
       release-$${subtarget}.commands = (cd $$subdir && $(MAKE) -f $(MAKEFILE) first)
       EXTRA_RELEASE_TARGETS += release-$${subtarget}
       QMAKE_EXTRA_TARGETS += release-$${subtarget}
   }
}
debug.depends = $$EXTRA_DEBUG_TARGETS
release.depends = $$EXTRA_RELEASE_TARGETS
QMAKE_EXTRA_TARGETS += debug release

SUBDIRS += $$SRC_SUBDIRS

#####################################################################
# Main projectfile
#####################################################################

load(qt_parts)

SUBDIRS += qmake/qmake-docs.pro

cross_compile: CONFIG += nostrip

confclean.depends += clean
confclean.commands =
unix {
  confclean.commands += (cd config.tests/unix/stl && $(MAKE) distclean); \
			(cd config.tests/unix/ptrsize && $(MAKE) distclean); \
			(cd config.tests/x11/notype && $(MAKE) distclean); \
			(cd config.tests/unix/getaddrinfo && $(MAKE) distclean); \
			(cd config.tests/unix/cups && $(MAKE) distclean); \
			(cd config.tests/unix/psql && $(MAKE) distclean); \
			(cd config.tests/unix/mysql && $(MAKE) distclean); \
 	 		(cd config.tests/unix/mysql_r && $(MAKE) distclean); \
			(cd config.tests/unix/nis && $(MAKE) distclean); \
			(cd config.tests/unix/iodbc && $(MAKE) distclean); \
			(cd config.tests/unix/odbc && $(MAKE) distclean); \
			(cd config.tests/unix/oci && $(MAKE) distclean); \
			(cd config.tests/unix/tds && $(MAKE) distclean); \
			(cd config.tests/unix/db2 && $(MAKE) distclean); \
			(cd config.tests/unix/ibase && $(MAKE) distclean); \
			(cd config.tests/unix/ipv6ifname && $(MAKE) distclean); \
			(cd config.tests/unix/zlib && $(MAKE) distclean); \
			(cd config.tests/unix/sqlite2 && $(MAKE) distclean); \
			(cd config.tests/unix/libjpeg && $(MAKE) distclean); \
			(cd config.tests/unix/libpng && $(MAKE) distclean); \
                        (cd config.tests/unix/slog2 && $(MAKE) distclean); \
			(cd config.tests/x11/xcursor && $(MAKE) distclean); \
			(cd config.tests/x11/xrender && $(MAKE) distclean); \
			(cd config.tests/x11/xrandr && $(MAKE) distclean); \
			(cd config.tests/x11/xkb && $(MAKE) distclean); \
			(cd config.tests/x11/xinput && $(MAKE) distclean); \
			(cd config.tests/x11/fontconfig && $(MAKE) distclean); \
			(cd config.tests/x11/xinerama && $(MAKE) distclean); \
			(cd config.tests/x11/xshape && $(MAKE) distclean); \
			(cd config.tests/x11/opengl && $(MAKE) distclean); \
                        $(DEL_FILE) config.tests/.qmake.cache; \
			$(DEL_FILE) src/corelib/global/qconfig.h; \
			$(DEL_FILE) src/corelib/global/qconfig.cpp; \
			$(DEL_FILE) mkspecs/qconfig.pri; \
			$(DEL_FILE) mkspecs/qdevice.pri; \
			$(DEL_FILE) mkspecs/qmodule.pri; \
			$(DEL_FILE) .qmake.cache; \
 			(cd qmake && $(MAKE) distclean);
}
win32 {
  confclean.commands += -$(DEL_FILE) src\\corelib\\global\\qconfig.h $$escape_expand(\\n\\t) \
			-$(DEL_FILE) src\\corelib\\global\\qconfig.cpp $$escape_expand(\\n\\t) \
			-$(DEL_FILE) mkspecs\\qconfig.pri $$escape_expand(\\n\\t) \
			-$(DEL_FILE) mkspecs\\qdevice.pri $$escape_expand(\\n\\t) \
			-$(DEL_FILE) mkspecs\\qmodule.pri $$escape_expand(\\n\\t) \
			-$(DEL_FILE) .qmake.cache $$escape_expand(\\n\\t) \
			(cd qmake && $(MAKE) distclean)
}
QMAKE_EXTRA_TARGETS += confclean
qmakeclean.commands += (cd qmake && $(MAKE) clean)
QMAKE_EXTRA_TARGETS += qmakeclean
CLEAN_DEPS += qmakeclean

CONFIG -= qt

### installations ####

#qmake
qmake.path = $$[QT_HOST_BINS]
equals(QMAKE_HOST.os, Windows) {
   qmake.files = $$OUT_PWD/bin/qmake.exe
} else {
   qmake.files = $$OUT_PWD/bin/qmake
}
INSTALLS += qmake

#syncqt
syncqt.path = $$[QT_HOST_BINS]
syncqt.files = $$PWD/bin/syncqt.pl
INSTALLS += syncqt

# If we are doing a prefix build, create a "module" pri which enables
# qtPrepareTool() to find the non-installed syncqt.
prefix_build|!equals(PWD, $$OUT_PWD) {

    cmd = perl -w $$shell_path($$PWD/bin/syncqt.pl)

    TOOL_PRI = $$OUT_PWD/mkspecs/modules/qt_tool_syncqt.pri

    TOOL_PRI_CONT = "QT_TOOL.syncqt.binary = $$val_escape(cmd)"
    write_file($$TOOL_PRI, TOOL_PRI_CONT)|error("Aborting.")

    # Then, inject the new tool into the current cache state
    !contains(QMAKE_INTERNAL_INCLUDED_FILES, $$TOOL_PRI) { # before the actual include()!
        added = $$TOOL_PRI
        cache(QMAKE_INTERNAL_INCLUDED_FILES, add transient, added)
    }
    include($$TOOL_PRI)
    cache(QT_TOOL.syncqt.binary, transient)

}

#mkspecs
mkspecs.path = $$[QT_HOST_DATA]/mkspecs
mkspecs.files = \
    $$OUT_PWD/mkspecs/qconfig.pri $$OUT_PWD/mkspecs/qmodule.pri $$OUT_PWD/mkspecs/qdevice.pri \
    $$files($$PWD/mkspecs/*)   # $$OUT_PWD contains only symlinks under Unix
mkspecs.files -= $$PWD/mkspecs/modules
INSTALLS += mkspecs

global_docs.files = $$PWD/doc/global
global_docs.path = $$[QT_INSTALL_DOCS]
INSTALLS += global_docs

OTHER_FILES += \
    configure \
    header.BSD \
    header.FDL \
    header.LGPL \
    header.LGPL-ONLY \
    sync.profile

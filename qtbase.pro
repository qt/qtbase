#####################################################################
# Main projectfile
#####################################################################

load(qt_parts)

SUBDIRS += qmake/qmake-aux.pro doc

cross_compile: CONFIG += nostrip

confclean.depends += distclean
confclean.commands = echo The confclean target is obsolete. Please use distclean instead.
QMAKE_EXTRA_TARGETS += confclean

qmake-clean.commands += (cd qmake && $(MAKE) clean)
QMAKE_EXTRA_TARGETS += qmake-clean
CLEAN_DEPS += qmake-clean

# We don't distclean qmake, as it may be needed for rebuilding Makefiles as a
# recursive distclean proceeds, including beyond qtbase.
DISTCLEAN_DEPS += qmake-clean

# Files created by configure.
# config.status (and configure.cache, which is the same for Windows)
# are omitted for convenience of rebuilds.
QMAKE_DISTCLEAN += \
    config.summary \
    config.tests/.qmake.cache \
    mkspecs/qconfig.pri \
    mkspecs/qdevice.pri \
    mkspecs/qmodule.pri \
    src/corelib/global/qconfig.h \
    src/corelib/global/qconfig_p.h \
    src/corelib/global/qconfig.cpp \
    bin/qt.conf

CONFIG -= qt

### installations ####

#licheck
licheck.path = $$[QT_HOST_BINS]
licheck.files = $$PWD/bin/$$QT_LICHECK
!isEmpty(QT_LICHECK): INSTALLS += licheck

#fixqt4headers.pl
fixqt4headers.path = $$[QT_HOST_BINS]
fixqt4headers.files = $$PWD/bin/fixqt4headers.pl
INSTALLS += fixqt4headers

#syncqt
syncqt.path = $$[QT_HOST_BINS]
syncqt.files = $$PWD/bin/syncqt.pl
INSTALLS += syncqt

# If we are doing a prefix build, create a "module" pri which enables
# qtPrepareTool() to find the non-installed syncqt.
prefix_build|!equals(PWD, $$OUT_PWD) {

    cmd = perl -w $$system_path($$PWD/bin/syncqt.pl)

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
    $$OUT_PWD/mkspecs/qconfig.pri $$OUT_PWD/mkspecs/qmodule.pri \
    $$OUT_PWD/mkspecs/qdevice.pri \
    $$files($$PWD/mkspecs/*)
mkspecs.files -= $$PWD/mkspecs/modules $$PWD/mkspecs/modules-inst
INSTALLS += mkspecs

OTHER_FILES += \
    configure \
    header.BSD \
    header.FDL \
    header.LGPL \
    header.LGPL-ONLY \
    sync.profile

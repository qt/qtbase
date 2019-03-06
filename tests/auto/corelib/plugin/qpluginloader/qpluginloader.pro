QT = core
TEMPLATE = subdirs

tst.depends = lib theplugin
SUBDIRS = lib \
          staticplugin \
          theplugin \
          tst
!android:!win32:!darwin {
    tst.depends += almostplugin
    SUBDIRS += almostplugin
}
macos:qtConfig(private_tests):qtHaveModule(gui) {
    tst.depends += machtest
    SUBDIRS += machtest
}

# no special install rule for subdir
INSTALLS =



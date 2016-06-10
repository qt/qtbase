TEMPLATE = subdirs
TESTPLUGINS =

win32 {
    contains(QT_CONFIG, debug): TESTPLUGINS += debugplugin
    contains(QT_CONFIG, release): TESTPLUGINS += releaseplugin
} else:osx {
    CONFIG(debug, debug|release): TESTPLUGINS += debugplugin
    CONFIG(release, debug|release): TESTPLUGINS += releaseplugin
} else {
    TESTPLUGINS = debugplugin releaseplugin
}

SUBDIRS += main $$TESTPLUGINS
main.file = tst_qplugin.pro
main.depends = $$TESTPLUGINS


TEMPLATE = subdirs

load(qfeatures)
SUBDIRS *= sqldrivers
qtHaveModule(network):!contains(QT_DISABLED_FEATURES, bearermanagement): SUBDIRS += bearer
qtHaveModule(gui) {
    SUBDIRS *= platforms platforminputcontexts platformthemes
    !contains(QT_DISABLED_FEATURES, imageformatplugin): SUBDIRS *= imageformats
    !contains(QT_DISABLED_FEATURES, library): SUBDIRS *= generic
}
qtHaveModule(widgets): SUBDIRS *= styles

!winrt:!wince*:qtHaveModule(widgets):!contains(QT_DISABLED_FEATURES, printer) {
    SUBDIRS += printsupport
}

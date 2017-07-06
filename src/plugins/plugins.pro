TEMPLATE = subdirs
QT_FOR_CONFIG += network

qtHaveModule(sql): SUBDIRS += sqldrivers
qtHaveModule(network):qtConfig(bearermanagement): SUBDIRS += bearer
qtHaveModule(gui) {
    SUBDIRS *= platforms platforminputcontexts platformthemes
    qtConfig(imageformatplugin): SUBDIRS *= imageformats
    !android:qtConfig(library): SUBDIRS *= generic
}
qtHaveModule(widgets): SUBDIRS += styles

!winrt:qtHaveModule(printsupport): \
    SUBDIRS += printsupport

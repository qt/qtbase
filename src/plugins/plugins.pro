TEMPLATE = subdirs
QT_FOR_CONFIG += network

SUBDIRS *= sqldrivers
qtHaveModule(network):qtConfig(bearermanagement): SUBDIRS += bearer
qtHaveModule(gui) {
    SUBDIRS *= platforms platforminputcontexts platformthemes
    qtConfig(imageformatplugin): SUBDIRS *= imageformats
    !android:qtConfig(library): SUBDIRS *= generic
}

!winrt:!wince:qtHaveModule(printsupport): \
    SUBDIRS += printsupport

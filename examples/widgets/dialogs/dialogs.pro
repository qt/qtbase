QT_FOR_CONFIG += widgets

TEMPLATE      = subdirs
SUBDIRS       = classwizard \
                configdialog \
                standarddialogs \
                tabdialog \
                trivialwizard

!wince {
    SUBDIRS += \
        licensewizard \
        extension \
        findfiles
}

!qtHaveModule(printsupport): SUBDIRS -= licensewizard
!qtConfig(wizard) {
    SUBDIRS -= trivialwizard licensewizard classwizard
}
wince: SUBDIRS += sipdialog

TEMPLATE      = subdirs
SUBDIRS       = classwizard \
                configdialog \
                standarddialogs \
                tabdialog \
                trivialwizard

!wince*: SUBDIRS += licensewizard \
                    extension \
                    findfiles

!qtHaveModule(printsupport): SUBDIRS -= licensewizard
contains(DEFINES, QT_NO_WIZARD): SUBDIRS -= trivialwizard licensewizard classwizard
wince*: SUBDIRS += sipdialog

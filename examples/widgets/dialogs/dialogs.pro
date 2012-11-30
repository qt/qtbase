TEMPLATE      = subdirs
SUBDIRS       = classwizard \
                configdialog \
                standarddialogs \
                tabdialog \
                trivialwizard

!wince*: SUBDIRS += licensewizard \
                    extension \
                    findfiles

contains(DEFINES, QT_NO_WIZARD): SUBDIRS -= trivialwizard licensewizard classwizard
wince*: SUBDIRS += sipdialog

QT += widgets

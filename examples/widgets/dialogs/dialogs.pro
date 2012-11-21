TEMPLATE      = subdirs
SUBDIRS       = classwizard \
                configdialog \
                standarddialogs \
                tabdialog \
                trivialwizard

!wince*: SUBDIRS += licensewizard \
                    extension \
                    findfiles

wince*: SUBDIRS += sipdialog

# install
sources.files = README *.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/widgets/dialogs
INSTALLS += sources

QT += widgets

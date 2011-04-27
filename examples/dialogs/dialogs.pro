TEMPLATE      = subdirs
SUBDIRS       = classwizard \
                configdialog \
                standarddialogs \
                tabdialog \
                trivialwizard

!symbian:!wince*: SUBDIRS += licensewizard \
                    extension \
                    findfiles

wince*: SUBDIRS += sipdialog

# install
sources.files = README *.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/dialogs
INSTALLS += sources

symbian: CONFIG += qt_example

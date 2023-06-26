TEMPLATE      = subdirs
SUBDIRS       = addressbook \
                basicsortfiltermodel \
                coloreditorfactory \
                combowidgetmapper \
                customsortfiltermodel \
                dirview \
                editabletreemodel \
                fetchmore \
                frozencolumn \
                interview \
                pixelator \
                puzzle \
                simpledommodel \
                simpletreemodel \
                simplewidgetmapper \
                spinboxdelegate \
                spreadsheet \
                stardelegate
!qtConfig(draganddrop): SUBDIRS -= puzzle
!qtHaveModule(xml): SUBDIRS -= simpledommodel

TEMPLATE      = subdirs
SUBDIRS       = addressbook \
                basicsortfiltermodel \
                coloreditorfactory \
                combowidgetmapper \
                customsortfiltermodel \
                editabletreemodel \
                fetchmore \
                frozencolumn \
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

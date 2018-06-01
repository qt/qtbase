TEMPLATE      = subdirs
SUBDIRS       = addressbook \
                basicsortfiltermodel \
                chart \
                coloreditorfactory \
                combowidgetmapper \
                customsortfiltermodel \
                dirview \
                editabletreemodel \
                fetchmore \
                flattreeview \
                frozencolumn \
                interview \
                pixelator \
                puzzle \
                simpledommodel \
                simpletreemodel \
                simplewidgetmapper \
                spinboxdelegate \
                spreadsheet \
                stardelegate \
                storageview
!qtConfig(draganddrop): SUBDIRS -= puzzle
!qtHaveModule(xml): SUBDIRS -= simpledommodel

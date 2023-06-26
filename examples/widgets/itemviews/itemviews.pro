TEMPLATE      = subdirs
SUBDIRS       = addressbook \
                basicsortfiltermodel \
                coloreditorfactory \
                combowidgetmapper \
                customsortfiltermodel \
                editabletreemodel \
                fetchmore \
                frozencolumn \
                simpledommodel \
                simpletreemodel \
                simplewidgetmapper \
                spinboxdelegate \
                spreadsheet \
                stardelegate
!qtHaveModule(xml): SUBDIRS -= simpledommodel

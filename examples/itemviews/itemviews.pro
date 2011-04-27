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
                frozencolumn \
                pixelator \
                puzzle \
                simpledommodel \
                simpletreemodel \
                simplewidgetmapper \
                spinboxdelegate \
                stardelegate

symbian: SUBDIRS = \
                addressbook \
                chart

# install
sources.files = README *.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/itemviews
INSTALLS += sources

symbian: CONFIG += qt_example

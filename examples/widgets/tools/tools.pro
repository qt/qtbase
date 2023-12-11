TEMPLATE      = subdirs
SUBDIRS       = \
                completer \
                customcompleter \
                echoplugin \
                regularexpression \
                styleplugin \
                treemodelcompleter \
                undoframework

!qtConfig(library) {
    SUBDIRS -= \
        echoplugin
}

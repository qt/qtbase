TEMPLATE      = subdirs
SUBDIRS       = \
                completer \
                customcompleter \
                echoplugin \
                regularexpression \
                settingseditor \
                styleplugin \
                treemodelcompleter \
                undoframework

!qtConfig(library) {
    SUBDIRS -= \
        echoplugin
}

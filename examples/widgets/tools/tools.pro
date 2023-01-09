TEMPLATE      = subdirs
SUBDIRS       = \
                completer \
                customcompleter \
                echoplugin \
                plugandpaint \
                regularexpression \
                settingseditor \
                styleplugin \
                treemodelcompleter \
                undoframework

!qtConfig(library) {
    SUBDIRS -= \
        echoplugin \
        plugandpaint
}

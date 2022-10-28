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
                undo \
                undoframework

!qtConfig(library) {
    SUBDIRS -= \
        echoplugin \
        plugandpaint
}

TEMPLATE      = subdirs
SUBDIRS       = \
                completer \
                customcompleter \
                echoplugin \
                i18n \
                plugandpaint \
                regularexpression \
                settingseditor \
                styleplugin \
                treemodelcompleter \
                undo \
                undoframework

contains(DEFINES, QT_NO_TRANSLATION): SUBDIRS -= i18n

!qtConfig(library) {
    SUBDIRS -= \
        echoplugin \
        plugandpaint
}

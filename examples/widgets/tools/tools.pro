TEMPLATE      = subdirs
SUBDIRS       = \
                codecs \
                completer \
                customcompleter \
                echoplugin \
                i18n \
                plugandpaint \
                regexp \
                regularexpression \
                settingseditor \
                styleplugin \
                treemodelcompleter \
                undo \
                undoframework

contains(DEFINES, QT_NO_TRANSLATION): SUBDIRS -= i18n

load(qfeatures)
contains(QT_DISABLED_FEATURES, library) {
    SUBDIRS -= \
        echoplugin \
        plugandpaint
}

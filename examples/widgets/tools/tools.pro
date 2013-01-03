TEMPLATE      = subdirs
SUBDIRS       = \
                codecs \
                completer \
                customcompleter \
                echoplugin \
                i18n \
                plugandpaint \
                plugandpaintplugins \
                regexp \
                settingseditor \
                styleplugin \
                treemodelcompleter \
                undo \
                undoframework

contains(DEFINES, QT_NO_TRANSLATION): SUBDIRS -= i18n

plugandpaint.depends = plugandpaintplugins

# install
sources.files = tools.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/widgets/tools
INSTALLS += sources

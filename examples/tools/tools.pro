TEMPLATE      = subdirs
CONFIG       += ordered
SUBDIRS       = codecs \
                completer \
                customcompleter \
                echoplugin \
                i18n \
                contiguouscache \
                plugandpaintplugins \
                plugandpaint \
                regexp \
                settingseditor \
                styleplugin \
                treemodelcompleter \
                undo \
                undoframework

plugandpaint.depends = plugandpaintplugins

# install
target.path = $$[QT_INSTALL_EXAMPLES]/tools
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS tools.pro README
sources.path = $$[QT_INSTALL_EXAMPLES]/tools
INSTALLS += target sources

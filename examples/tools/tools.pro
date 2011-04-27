TEMPLATE      = subdirs
CONFIG       += ordered
SUBDIRS       = codecs \
                completer \
                customcompleter \
                echoplugin \
                i18n \
                inputpanel \
                contiguouscache \
                plugandpaintplugins \
                plugandpaint \
                regexp \
                settingseditor \
                styleplugin \
                treemodelcompleter \
                undoframework

plugandpaint.depends = plugandpaintplugins

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/tools
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS tools.pro README
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/tools
INSTALLS += target sources

symbian: include($$QT_SOURCE_TREE/examples/symbianpkgrules.pri)

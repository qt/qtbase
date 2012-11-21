#! [0]
TEMPLATE      = lib
CONFIG       += plugin
INCLUDEPATH  += ../..
HEADERS       = extrafiltersplugin.h
SOURCES       = extrafiltersplugin.cpp
OTHER_FILES  += extrafilters.json
TARGET        = $$qtLibraryTarget(pnp_extrafilters)
DESTDIR       = ../../plugandpaint/plugins

#! [0]
# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/tools/plugandpaint/plugins
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS extrafilters.pro extrafilters.json
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/tools/plugandpaintplugins/extrafilters
INSTALLS += target sources


QT += widgets

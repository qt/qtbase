#! [0]
TEMPLATE      = lib
CONFIG       += plugin static
INCLUDEPATH  += ../..
HEADERS       = basictoolsplugin.h
SOURCES       = basictoolsplugin.cpp
OTHER_FILES  += basictools.json
TARGET        = $$qtLibraryTarget(pnp_basictools)
DESTDIR       = ../../plugandpaint/plugins
#! [0]

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/tools/plugandpaint/plugins
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS basictools.pro basictools.json
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/tools/plugandpaintplugins/basictools
INSTALLS += target sources

QT += widgets

#! [0]
TEMPLATE      = lib
CONFIG       += plugin static
QT           += widgets
INCLUDEPATH  += ../..
HEADERS       = basictoolsplugin.h
SOURCES       = basictoolsplugin.cpp
TARGET        = $$qtLibraryTarget(pnp_basictools)
DESTDIR       = ../../plugandpaint/plugins
#! [0]

# install
target.path = $$[QT_INSTALL_EXAMPLES]/tools/plugandpaint/plugins
INSTALLS += target

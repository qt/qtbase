TEMPLATE    = lib
CONFIG     += plugin

TARGET      = dbscreen
target.path += $$[QT_INSTALL_PLUGINS]/gfxdrivers
INSTALLS    += target

HEADERS     = dbscreen.h 
SOURCES     = dbscreendriverplugin.cpp \
              dbscreen.cpp 

QT += widgets
symbian: CONFIG += qt_example
maemo5: CONFIG += qt_example
symbian: warning(This example does not work on Symbian platform)
simulator: warning(This example does not work on Simulator platform)

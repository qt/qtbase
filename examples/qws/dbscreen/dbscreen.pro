TEMPLATE    = lib
CONFIG     += plugin

TARGET      = dbscreen
target.path += $$[QT_INSTALL_PLUGINS]/gfxdrivers
INSTALLS    += target

HEADERS     = dbscreen.h 
SOURCES     = dbscreendriverplugin.cpp \
              dbscreen.cpp 


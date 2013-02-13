requires(if(wince|embedded|x11):qtHaveModule(gui))

TEMPLATE  = subdirs
SUBDIRS   = styleexample raycasting flickable digiflip

SUBDIRS += lightmaps
SUBDIRS += flightinfo

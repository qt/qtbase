TEMPLATE  = subdirs
SUBDIRS   = styleexample raycasting flickable digiflip

SUBDIRS += lightmaps
SUBDIRS += flightinfo

# install
sources.files = README *.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/embedded
INSTALLS += sources

QT += widgets widgets

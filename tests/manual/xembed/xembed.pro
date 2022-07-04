TEMPLATE=subdirs
QT_FOR_CONFIG += network-private gui-private

TEMPLATE = subdirs
QT_FOR_CONFIG += widgets-private

SUBDIRS = \
qt-client-raster \
qt-client-widget

qtHaveModule(widgets):qtConfig(gtk3): SUBDIRS += gtk-container

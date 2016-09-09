TEMPLATE = subdirs
QT_FOR_CONFIG += widgets-private

qtHaveModule(widgets):qtConfig(gtk3): SUBDIRS += gtk3

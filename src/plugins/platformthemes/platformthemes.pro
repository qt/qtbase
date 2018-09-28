TEMPLATE = subdirs
QT_FOR_CONFIG += widgets-private

qtConfig(dbus):qtConfig(regularexpression):qtConfig(mimetype): SUBDIRS += flatpak

qtHaveModule(widgets):qtConfig(gtk3): SUBDIRS += gtk3

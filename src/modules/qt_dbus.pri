QT_DBUS_VERSION = $$QT_VERSION
QT_DBUS_MAJOR_VERSION = $$QT_MAJOR_VERSION
QT_DBUS_MINOR_VERSION = $$QT_MINOR_VERSION
QT_DBUS_PATCH_VERSION = $$QT_PATCH_VERSION

QT.dbus.name = QtDBus
QT.dbus.includes = $$QT_MODULE_INCLUDE_BASE/QtDBus
QT.dbus.private_includes = $$QT_MODULE_INCLUDE_BASE/QtDBus/private
QT.dbus.sources = $$QT_MODULE_BASE/src/dbus
QT.dbus.libs = $$QT_MODULE_LIB_BASE
QT.dbus.depends = core xml
QT.dbus.CONFIG = dbusadaptors dbusinterfaces

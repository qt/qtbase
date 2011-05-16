QT.dbus.VERSION = 5.0.0
QT.dbus.MAJOR_VERSION = 5
QT.dbus.MINOR_VERSION = 0
QT.dbus.PATCH_VERSION = 0

QT.dbus.name = QtDBus
QT.dbus.bins = $$QT_MODULE_BIN_BASE
QT.dbus.includes = $$QT_MODULE_INCLUDE_BASE/QtDBus
QT.dbus.private_includes = $$QT_MODULE_INCLUDE_BASE/QtDBus/$$QT.dbus.VERSION
QT.dbus.sources = $$QT_MODULE_BASE/src/dbus
QT.dbus.libs = $$QT_MODULE_LIB_BASE
QT.dbus.plugins = $$QT_MODULE_PLUGIN_BASE
QT.dbus.imports = $$QT_MODULE_IMPORT_BASE
QT.dbus.depends = core xml
QT.dbus.CONFIG = dbusadaptors dbusinterfaces
QT.dbus.DEFINES = QT_DBUS_LIB

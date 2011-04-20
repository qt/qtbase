QT.core.VERSION = 4.8.0
QT.core.MAJOR_VERSION = 4
QT.core.MINOR_VERSION = 8
QT.core.PATCH_VERSION = 0

QT.core.name = QtCore
QT.core.bins = $$QT_MODULE_BIN_BASE
QT.core.includes = $$QT_MODULE_INCLUDE_BASE/QtCore
QT.core.private_includes = $$QT_MODULE_INCLUDE_BASE/QtCore/$$QT.core.VERSION
QT.core.sources = $$QT_MODULE_BASE/src/corelib
QT.core.libs = $$QT_MODULE_LIB_BASE
QT.core.plugins = $$QT_MODULE_PLUGIN_BASE
QT.core.imports = $$QT_MODULE_IMPORT_BASE
QT.core.depends =
QT.core.DEFINES = QT_CORE_LIB

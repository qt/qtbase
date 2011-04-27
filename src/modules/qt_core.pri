QT_CORE_VERSION = $$QT_VERSION
QT_CORE_MAJOR_VERSION = $$QT_MAJOR_VERSION
QT_CORE_MINOR_VERSION = $$QT_MINOR_VERSION
QT_CORE_PATCH_VERSION = $$QT_PATCH_VERSION

QT.core.name = QtCore
QT.core.bins = $$QT_MODULE_BIN_BASE
QT.core.includes = $$QT_MODULE_INCLUDE_BASE/QtCore
QT.core.private_includes = $$QT_MODULE_INCLUDE_BASE/QtCore/private
QT.core.sources = $$QT_MODULE_BASE/src/corelib
QT.core.libs = $$QT_MODULE_LIB_BASE
QT.core.plugins = $$QT_MODULE_PLUGIN_BASE
QT.core.imports = $$QT_MODULE_IMPORT_BASE
QT.core.depends =
QT.core.DEFINES = QT_CORE_LIB

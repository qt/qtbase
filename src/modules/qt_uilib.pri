QT.uilib.VERSION = 4.8.0
QT.uilib.MAJOR_VERSION = 4
QT.uilib.MINOR_VERSION = 8
QT.uilib.PATCH_VERSION = 0

QT.uilib.name = QtUiLib
QT.uilib.bins = $$QT_MODULE_BIN_BASE
QT.uilib.includes = $$QT_MODULE_INCLUDE_BASE/QtDesigner
QT.uilib.private_includes = $$QT_MODULE_INCLUDE_BASE/QtDesigner/$$QT.uilib.VERSION
QT.uilib.sources = $$QT_MODULE_BASE/tools/uilib
QT.uilib.plugins = $$QT_MODULE_PLUGIN_BASE
QT.uilib.imports = $$QT_MODULE_IMPORT_BASE
QT.uilib.depends = xml

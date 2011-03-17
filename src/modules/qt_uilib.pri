QT_UILIB_VERSION = $$QT_VERSION
QT_UILIB_MAJOR_VERSION = $$QT_MAJOR_VERSION
QT_UILIB_MINOR_VERSION = $$QT_MINOR_VERSION
QT_UILIB_PATCH_VERSION = $$QT_PATCH_VERSION

QT.uilib.name =
QT.uilib.bins = $$QT_MODULE_BIN_BASE
QT.uilib.includes = $$QT_MODULE_INCLUDE_BASE/QtDesigner
QT.uilib.private_includes = $$QT_MODULE_INCLUDE_BASE/QtDesigner/private
QT.uilib.sources = $$QT_MODULE_BASE/tools/uilib
QT.uilib.libs = $$QT_MODULE_LIB_BASE
QT.uilib.imports = $$QT_MODULE_IMPORT_BASE
QT.uilib.depends = xml

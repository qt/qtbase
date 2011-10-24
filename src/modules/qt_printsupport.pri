QT.printsupport.VERSION = 5.0.0
QT.printsupport.MAJOR_VERSION = 5
QT.printsupport.MINOR_VERSION = 0
QT.printsupport.PATCH_VERSION = 0

QT.printsupport.name = QtPrintSupport
QT.printsupport.includes = $$QT_MODULE_INCLUDE_BASE/QtPrintSupport
QT.printsupport.private_includes = $$QT_MODULE_INCLUDE_BASE/QtPrintSupport/$$QT.printsupport.VERSION
QT.printsupport.sources = $$QT_MODULE_BASE/src/printsupport
QT.printsupport.libs = $$QT_MODULE_LIB_BASE
QT.printsupport.plugins = $$QT_MODULE_PLUGIN_BASE
QT.printsupport.imports = $$QT_MODULE_IMPORT_BASE
QT.printsupport.depends = core gui widgets
QT.printsupport.DEFINES = QT_PRINTSUPPORT_LIB

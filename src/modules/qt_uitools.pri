QT.uitools.VERSION = 4.8.0
QT.uitools.MAJOR_VERSION = 4
QT.uitools.MINOR_VERSION = 8
QT.uitools.PATCH_VERSION = 0

QT.uitools.name = QtUiTools
QT.uitools.bins = $$QT_MODULE_BIN_BASE
QT.uitools.includes = $$QT_MODULE_INCLUDE_BASE/QtUiTools
QT.uitools.private_includes = $$QT_MODULE_INCLUDE_BASE/QtUiTools/$$QT.uitools.VERSION
QT.uitools.sources = $$QT_MODULE_BASE/src/uitools
QT.uitools.libs = $$QT_MODULE_LIB_BASE
QT.uitools.plugins = $$QT_MODULE_PLUGIN_BASE
QT.uitools.imports = $$QT_MODULE_IMPORT_BASE
QT.uitools.depends = xml
QT.uitools.DEFINES = QT_UITOOLS_LIB

QT_CONFIG += uitools

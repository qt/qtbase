QT_UITOOLS_VERSION = $$QT_VERSION
QT_UITOOLS_MAJOR_VERSION = $$QT_MAJOR_VERSION
QT_UITOOLS_MINOR_VERSION = $$QT_MINOR_VERSION
QT_UITOOLS_PATCH_VERSION = $$QT_PATCH_VERSION

QT.uitools.name = QtUiTools
QT.uitools.bins = $$QT_MODULE_BIN_BASE
QT.uitools.includes = $$QT_MODULE_INCLUDE_BASE/QtUiTools
QT.uitools.private_includes = $$QT_MODULE_INCLUDE_BASE/QtUiTools/private
QT.uitools.sources = $$QT_MODULE_BASE/src/uitools
QT.uitools.libs = $$QT_MODULE_LIB_BASE
QT.uitools.plugins = $$QT_MODULE_PLUGIN_BASE
QT.uitools.imports = $$QT_MODULE_IMPORT_BASE
QT.uitools.depends = xml
QT.uitools.DEFINES = QT_UITOOLS_LIB

QT_CONFIG += uitools

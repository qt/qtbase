QT.openvg.VERSION = 4.8.0
QT.openvg.MAJOR_VERSION = 4
QT.openvg.MINOR_VERSION = 8
QT.openvg.PATCH_VERSION = 0

QT.openvg.name = QtOpenVG
QT.openvg.bins = $$QT_MODULE_BIN_BASE
QT.openvg.includes = $$QT_MODULE_INCLUDE_BASE/QtOpenVG
QT.openvg.private_includes = $$QT_MODULE_INCLUDE_BASE/QtOpenVG/$$QT.openvg.VERSION
QT.openvg.sources = $$QT_MODULE_BASE/src/openvg
QT.openvg.libs = $$QT_MODULE_LIB_BASE
QT.openvg.plugins = $$QT_MODULE_PLUGIN_BASE
QT.openvg.imports = $$QT_MODULE_IMPORT_BASE
QT.openvg.depends = core gui
QT.openvg.CONFIG = openvg
QT.openvg.DEFINES = QT_OPENVG_LIB

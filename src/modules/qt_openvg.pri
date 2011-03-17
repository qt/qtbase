QT_OPENVG_VERSION = $$QT_VERSION
QT_OPENVG_MAJOR_VERSION = $$QT_MAJOR_VERSION
QT_OPENVG_MINOR_VERSION = $$QT_MINOR_VERSION
QT_OPENVG_PATCH_VERSION = $$QT_PATCH_VERSION

QT.openvg.name = QtOpenVG
QT.openvg.bins = $$QT_MODULE_BIN_BASE
QT.openvg.includes = $$QT_MODULE_INCLUDE_BASE/QtOpenVG
QT.openvg.private_includes = $$QT_MODULE_INCLUDE_BASE/QtOpenVG/private
QT.openvg.sources = $$QT_MODULE_BASE/src/openvg
QT.openvg.libs = $$QT_MODULE_LIB_BASE
QT.openvg.imports = $$QT_MODULE_IMPORT_BASE
QT.openvg.depends = core gui
QT.openvg.CONFIG = openvg
QT.openvg.DEFINES = QT_OPENVG_LIB

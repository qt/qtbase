QT.opengl.VERSION = 5.0.0
QT.opengl.MAJOR_VERSION = 5
QT.opengl.MINOR_VERSION = 0
QT.opengl.PATCH_VERSION = 0

QT.opengl.name = QtOpenGL
QT.opengl.bins = $$QT_MODULE_BIN_BASE
QT.opengl.includes = $$QT_MODULE_INCLUDE_BASE/QtOpenGL
QT.opengl.private_includes = $$QT_MODULE_INCLUDE_BASE/QtOpenGL/$$QT.opengl.VERSION
QT.opengl.sources = $$QT_MODULE_BASE/src/opengl
QT.opengl.libs = $$QT_MODULE_LIB_BASE
QT.opengl.plugins = $$QT_MODULE_PLUGIN_BASE
QT.opengl.imports = $$QT_MODULE_IMPORT_BASE
QT.opengl.depends = core gui
QT.opengl.CONFIG = opengl
QT.opengl.DEFINES = QT_OPENGL_LIB

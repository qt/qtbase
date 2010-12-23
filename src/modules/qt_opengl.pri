QT_OPENGL_VERSION = $$QT_VERSION
QT_OPENGL_MAJOR_VERSION = $$QT_MAJOR_VERSION
QT_OPENGL_MINOR_VERSION = $$QT_MINOR_VERSION
QT_OPENGL_PATCH_VERSION = $$QT_PATCH_VERSION

QT.opengl.name = QtOpenGL
QT.opengl.includes = $$QT_MODULE_INCLUDE_BASE/QtOpenGL
QT.opengl.private_includes = $$QT_MODULE_INCLUDE_BASE/QtOpenGL/private
QT.opengl.sources = $$QT_MODULE_BASE/src/opengl
QT.opengl.libs = $$QT_MODULE_LIB_BASE
QT.opengl.depends = core gui
QT.opengl.CONFIG = opengl

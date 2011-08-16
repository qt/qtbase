QT.gui.VERSION = 5.0.0
QT.gui.MAJOR_VERSION = 5
QT.gui.MINOR_VERSION = 0
QT.gui.PATCH_VERSION = 0

QT.gui.name = QtGui
QT.gui.includes = $$QT_MODULE_INCLUDE_BASE/QtGui
QT.gui.private_includes = $$QT_MODULE_INCLUDE_BASE/QtGui/$$QT.gui.VERSION
QT.gui.sources = $$QT_MODULE_BASE/src/gui
QT.gui.libs = $$QT_MODULE_LIB_BASE
QT.gui.plugins = $$QT_MODULE_PLUGIN_BASE
QT.gui.imports = $$QT_MODULE_IMPORT_BASE
QT.gui.depends = core
QT.gui.DEFINES = QT_GUI_LIB

QT_GUI_VERSION = $$QT_VERSION
QT_GUI_MAJOR_VERSION = $$QT_MAJOR_VERSION
QT_GUI_MINOR_VERSION = $$QT_MINOR_VERSION
QT_GUI_PATCH_VERSION = $$QT_PATCH_VERSION

QT.gui.name = QtGui
QT.gui.includes = $$QT_MODULE_INCLUDE_BASE/QtGui
QT.gui.private_includes = $$QT_MODULE_INCLUDE_BASE/QtGui/private
QT.gui.sources = $$QT_MODULE_BASE/src/gui
QT.gui.libs = $$QT_MODULE_LIB_BASE
QT.gui.plugins = $$QT_MODULE_PLUGIN_BASE
QT.gui.imports = $$QT_MODULE_IMPORT_BASE
QT.gui.depends = core network
QT.gui.DEFINES = QT_GUI_LIB

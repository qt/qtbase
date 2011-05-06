QT.widgets.VERSION = 4.8.0
QT.widgets.MAJOR_VERSION = 4
QT.widgets.MINOR_VERSION = 8
QT.widgets.PATCH_VERSION = 0

QT.widgets.name = QtGui
QT.widgets.includes = $$QT_MODULE_INCLUDE_BASE/QtWidgets
QT.widgets.private_includes = $$QT_MODULE_INCLUDE_BASE/QtWidgets/$$QT.widgets.VERSION
QT.widgets.sources = $$QT_MODULE_BASE/src/widgets
QT.widgets.libs = $$QT_MODULE_LIB_BASE
QT.widgets.plugins = $$QT_MODULE_PLUGIN_BASE
QT.widgets.imports = $$QT_MODULE_IMPORT_BASE
QT.widgets.depends = core network
QT.widgets.DEFINES = QT_GUI_LIB

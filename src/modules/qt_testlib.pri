QT.testlib.VERSION = 5.0.0
QT.testlib.MAJOR_VERSION = 5
QT.testlib.MINOR_VERSION = 0
QT.testlib.PATCH_VERSION = 0

QT.testlib.name = QtTest
QT.testlib.bins = $$QT_MODULE_BIN_BASE
QT.testlib.includes = $$QT_MODULE_INCLUDE_BASE/QtTest
QT.testlib.private_includes = $$QT_MODULE_INCLUDE_BASE/QtTest/$$QT.testlib.VERSION
QT.testlib.sources = $$QT_MODULE_BASE/src/testlib
QT.testlib.libs = $$QT_MODULE_LIB_BASE
QT.testlib.plugins = $$QT_MODULE_PLUGIN_BASE
QT.testlib.imports = $$QT_MODULE_IMPORT_BASE
QT.testlib.depends = core
QT.testlib.CONFIG = console
QT.testlib.DEFINES = QT_TESTLIB_LIB

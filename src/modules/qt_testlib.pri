QT_TEST_VERSION = $$QT_VERSION
QT_TEST_MAJOR_VERSION = $$QT_MAJOR_VERSION
QT_TEST_MINOR_VERSION = $$QT_MINOR_VERSION
QT_TEST_PATCH_VERSION = $$QT_PATCH_VERSION

QT.testlib.name = QtTest
QT.testlib.bins = $$QT_MODULE_BIN_BASE
QT.testlib.includes = $$QT_MODULE_INCLUDE_BASE/QtTest
QT.testlib.private_includes = $$QT_MODULE_INCLUDE_BASE/QtTest/private
QT.testlib.sources = $$QT_MODULE_BASE/src/testlib
QT.testlib.libs = $$QT_MODULE_LIB_BASE
QT.testlib.depends = core
QT.testlib.CONFIG = console
QT.testlib.DEFINES = QT_TESTLIB_LIB

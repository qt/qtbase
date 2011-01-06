QT_CORE_VERSION = $$QT_VERSION
QT_NETWORK_MAJOR_VERSION = $$QT_MAJOR_VERSION
QT_NETWORK_MINOR_VERSION = $$QT_MINOR_VERSION
QT_NETWORK_PATCH_VERSION = $$QT_PATCH_VERSION

QT.network.name = QtNetwork
QT.network.bins = $$QT_MODULE_BIN_BASE
QT.network.includes = $$QT_MODULE_INCLUDE_BASE/QtNetwork
QT.network.private_includes = $$QT_MODULE_INCLUDE_BASE/QtNetwork/private
QT.network.sources = $$QT_MODULE_BASE/src/network
QT.network.libs = $$QT_MODULE_LIB_BASE
QT.network.depends = core
QT.network.DEFINES = QT_NETWORK_LIB

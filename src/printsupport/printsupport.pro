load(qt_module)

TARGET     = QtPrintSupport
QPRO_PWD   = $$PWD
QT = core-private gui-private widgets-private

CONFIG += module
MODULE_PRI = ../modules/qt_printsupport.pri

DEFINES   += QT_BUILD_PRINTSUPPORT_LIB QT_NO_USING_NAMESPACE

unix|win32-g++*:QMAKE_PKGCONFIG_REQUIRES = QtCore QtGui

load(qt_module_config)

HEADERS += $$QT_SOURCE_TREE/src/printsupport/qtprintsupportversion.h

QMAKE_LIBS += $$QMAKE_LIBS_PRINTSUPPORT

include(kernel/kernel.pri)
include(widgets/widgets.pri)
include(dialogs/dialogs.pri)

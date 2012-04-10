load(qt_module)

TARGET     = QtPrintSupport
QPRO_PWD   = $$PWD
QT = core-private gui-private widgets-private

DEFINES   += QT_BUILD_PRINTSUPPORT_LIB QT_NO_USING_NAMESPACE

load(qt_module_config)

QMAKE_DOCS = $$PWD/doc/qtprintsupport.qdocconf
QMAKE_DOCS_INDEX = ../../doc

QMAKE_LIBS += $$QMAKE_LIBS_PRINTSUPPORT

include(kernel/kernel.pri)
include(widgets/widgets.pri)
include(dialogs/dialogs.pri)

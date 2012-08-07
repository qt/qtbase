load(qt_build_config)

TARGET     = QtPrintSupport
QT = core-private gui-private widgets-private

DEFINES   += QT_NO_USING_NAMESPACE

load(qt_module)

QMAKE_DOCS = $$PWD/doc/qtprintsupport.qdocconf
QMAKE_DOCS_INDEX = ../../doc

QMAKE_LIBS += $$QMAKE_LIBS_PRINTSUPPORT

include(kernel/kernel.pri)
include(widgets/widgets.pri)
include(dialogs/dialogs.pri)

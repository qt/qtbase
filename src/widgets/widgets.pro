TARGET     = QtWidgets
QT = core-private gui-private
MODULE_CONFIG = uic

CONFIG += $$MODULE_CONFIG
DEFINES   += QT_NO_USING_NAMESPACE
msvc:equals(QT_ARCH, i386): QMAKE_LFLAGS += /BASE:0x65000000

TRACEPOINT_PROVIDER = $$PWD/qtwidgets.tracepoints
CONFIG += qt_tracepoints

QMAKE_DOCS = $$PWD/doc/qtwidgets.qdocconf

#platforms
mac:include(kernel/mac.pri)
win32:include(kernel/win.pri)

#modules
include(kernel/kernel.pri)
include(styles/styles.pri)
include(widgets/widgets.pri)
include(dialogs/dialogs.pri)
include(accessible/accessible.pri)
include(itemviews/itemviews.pri)
include(graphicsview/graphicsview.pri)
include(util/util.pri)
include(statemachine/statemachine.pri)

qtConfig(graphicseffect) {
    include(effects/effects.pri)
}

QMAKE_LIBS += $$QMAKE_LIBS_GUI

QMAKE_DYNAMIC_LIST_FILE = $$PWD/QtWidgets.dynlist

# Code coverage with TestCocoon
# The following is required as extra compilers use $$QMAKE_CXX instead of $(CXX).
# Without this, testcocoon.prf is read only after $$QMAKE_CXX is used by the
# extra compilers.
testcocoon {
    load(testcocoon)
}

MODULE_PLUGIN_TYPES += \
    styles
load(qt_module)

CONFIG += metatypes install_metatypes

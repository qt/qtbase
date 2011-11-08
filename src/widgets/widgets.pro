load(qt_module)

TARGET     = QtWidgets
QPRO_PWD   = $$PWD
QT = core core-private gui gui-private

CONFIG += module
MODULE_PRI = ../modules/qt_widgets.pri

DEFINES   += QT_BUILD_WIDGETS_LIB QT_NO_USING_NAMESPACE
win32-msvc*|win32-icc:QMAKE_LFLAGS += /BASE:0x65000000
irix-cc*:QMAKE_CXXFLAGS += -no_prelink -ptused

unix|win32-g++*:QMAKE_PKGCONFIG_REQUIRES = QtCore

HEADERS += $$QT_SOURCE_TREE/src/widgets/qtwidgetsversion.h

include(../qbase.pri)

contains(QT_CONFIG, x11sm):CONFIG += x11sm

#platforms
x11:include(kernel/x11.pri)
mac:include(kernel/mac.pri)
win32:include(kernel/win.pri)

#modules
include(animation/animation.pri)
include(kernel/kernel.pri)
include(styles/styles.pri)
include(widgets/widgets.pri)
include(dialogs/dialogs.pri)
include(accessible/accessible.pri)
include(itemviews/itemviews.pri)
include(graphicsview/graphicsview.pri)
include(util/util.pri)
include(statemachine/statemachine.pri)
include(effects/effects.pri)


QMAKE_LIBS += $$QMAKE_LIBS_GUI

contains(DEFINES,QT_EVAL):include($$QT_SOURCE_TREE/src/corelib/eval.pri)

QMAKE_DYNAMIC_LIST_FILE = $$PWD/QtGui.dynlist

DEFINES += Q_INTERNAL_QAPP_SRC

INCLUDEPATH += ../3rdparty/harfbuzz/src

win32:!contains(QT_CONFIG, directwrite) {
    DEFINES += QT_NO_DIRECTWRITE
}

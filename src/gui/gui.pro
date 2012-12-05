TARGET     = QtGui
QT = core-private
contains(QT_CONFIG, opengl.*):MODULE_CONFIG = opengl

DEFINES   += QT_NO_USING_NAMESPACE

QMAKE_DOCS = $$PWD/doc/qtgui.qdocconf

load(qt_module)

# Code coverage with TestCocoon
# The following is required as extra compilers use $$QMAKE_CXX instead of $(CXX).
# Without this, testcocoon.prf is read only after $$QMAKE_CXX is used by the
# extra compilers.
testcocoon {
    load(testcocoon)
}

mac {
    LIBS_PRIVATE += -framework Cocoa
}

CONFIG += simd

include(accessible/accessible.pri)
include(kernel/kernel.pri)
include(image/image.pri)
include(text/text.pri)
include(painting/painting.pri)
include(util/util.pri)
include(math3d/math3d.pri)
include(opengl/opengl.pri)
include(animation/animation.pri)
include(itemmodels/itemmodels.pri)

QMAKE_LIBS += $$QMAKE_LIBS_GUI


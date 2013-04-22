TARGET     = QtGui
QT = core-private

MODULE_CONFIG = needs_qpa_plugin
contains(QT_CONFIG, opengl.*):MODULE_CONFIG += opengl

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

mac:!ios: LIBS_PRIVATE += -framework Cocoa

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

load(cmake_functions)

!contains(QT_CONFIG, angle) {
    contains(QT_CONFIG, opengles1) {
        CMAKE_GL_INCDIRS = $$cmakeTargetPaths($$QMAKE_INCDIR_OPENGL_ES1)
        CMAKE_GL_HEADER_NAME = GLES/gl.h
    } else:contains(QT_CONFIG, opengles2) {
        CMAKE_GL_INCDIRS = $$cmakeTargetPaths($$QMAKE_INCDIR_OPENGL_ES2)
        CMAKE_GL_HEADER_NAME = GLES2/gl2.h
    } else {
        CMAKE_GL_INCDIRS = $$cmakeTargetPaths($$QMAKE_INCDIR_OPENGL)
        CMAKE_GL_HEADER_NAME = GL/gl.h
        mac: CMAKE_GL_HEADER_NAME = gl.h
    }
}

QMAKE_DYNAMIC_LIST_FILE = $$PWD/QtGui.dynlist

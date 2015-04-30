TARGET     = QtGui
QT = core-private

qtConfig(opengl.*): MODULE_CONFIG = opengl

DEFINES   += QT_NO_USING_NAMESPACE QT_NO_FOREACH

QMAKE_DOCS = $$PWD/doc/qtgui.qdocconf

MODULE_PLUGIN_TYPES = \
    accessiblebridge \
    platforms \
    platforms/darwin \
    xcbglintegrations \
    platformthemes \
    platforminputcontexts \
    generic \
    iconengines \
    imageformats \
    egldeviceintegrations

# This is here only because the platform plugin is no module, obviously.
qtConfig(angle) {
    MODULE_AUX_INCLUDES = \
        \$\$QT_MODULE_INCLUDE_BASE/QtANGLE
}

# Code coverage with TestCocoon
# The following is required as extra compilers use $$QMAKE_CXX instead of $(CXX).
# Without this, testcocoon.prf is read only after $$QMAKE_CXX is used by the
# extra compilers.
testcocoon {
    load(testcocoon)
}

osx: LIBS_PRIVATE += -framework AppKit
darwin: LIBS_PRIVATE += -framework CoreGraphics

CONFIG += simd optimize_full

include(accessible/accessible.pri)
include(kernel/kernel.pri)
include(image/image.pri)
include(text/text.pri)
include(painting/painting.pri)
include(util/util.pri)
include(math3d/math3d.pri)
include(opengl/opengl.pri)
qtConfig(animation): include(animation/animation.pri)
include(itemmodels/itemmodels.pri)
include(vulkan/vulkan.pri)
include(platform/platform.pri)
include(rhi/rhi.pri)

QMAKE_LIBS += $$QMAKE_LIBS_GUI

load(qt_module)
load(cmake_functions)

win32: CMAKE_WINDOWS_BUILD = True

qtConfig(angle) {
    !mingw|qtConfig(debug_and_release): debug_suffix="d"
    CMAKE_GL_INCDIRS = $$CMAKE_INCLUDE_DIR
    CMAKE_ANGLE_EGL_DLL_RELEASE = libEGL.dll
    CMAKE_ANGLE_EGL_IMPLIB_RELEASE = libEGL.$${QMAKE_EXTENSION_STATICLIB}
    CMAKE_ANGLE_GLES2_DLL_RELEASE = libGLESv2.dll
    CMAKE_ANGLE_GLES2_IMPLIB_RELEASE = libGLESv2.$${QMAKE_EXTENSION_STATICLIB}
    CMAKE_ANGLE_EGL_DLL_DEBUG = libEGL$${debug_suffix}.dll
    CMAKE_ANGLE_EGL_IMPLIB_DEBUG = libEGL$${debug_suffix}.$${QMAKE_EXTENSION_STATICLIB}
    CMAKE_ANGLE_GLES2_DLL_DEBUG = libGLESv2$${debug_suffix}.dll
    CMAKE_ANGLE_GLES2_IMPLIB_DEBUG = libGLESv2$${debug_suffix}.$${QMAKE_EXTENSION_STATICLIB}

    CMAKE_QT_OPENGL_IMPLEMENTATION = GLESv2
} else {
    qtConfig(egl) {
        CMAKE_EGL_LIBS = $$cmakeProcessLibs($$QMAKE_LIBS_EGL)
        !isEmpty(QMAKE_LIBDIR_EGL): CMAKE_EGL_LIBDIR += $$cmakeTargetPath($$QMAKE_LIBDIR_EGL)
    }

    qtConfig(opengles2) {
        !isEmpty(QMAKE_INCDIR_OPENGL_ES2): CMAKE_GL_INCDIRS = $$cmakeTargetPaths($$QMAKE_INCDIR_OPENGL_ES2)
        CMAKE_OPENGL_INCDIRS = $$cmakePortablePaths($$QMAKE_INCDIR_OPENGL_ES2)
        CMAKE_OPENGL_LIBS = $$cmakeProcessLibs($$QMAKE_LIBS_OPENGL_ES2)
        !isEmpty(QMAKE_LIBDIR_OPENGL_ES2): CMAKE_OPENGL_LIBDIR = $$cmakePortablePaths($$QMAKE_LIBDIR_OPENGL_ES2)
        CMAKE_GL_HEADER_NAME = GLES2/gl2.h
        CMAKE_QT_OPENGL_IMPLEMENTATION = GLESv2
    } else: qtConfig(opengl) {
        !isEmpty(QMAKE_INCDIR_OPENGL): CMAKE_GL_INCDIRS = $$cmakeTargetPaths($$QMAKE_INCDIR_OPENGL)
        CMAKE_OPENGL_INCDIRS = $$cmakePortablePaths($$QMAKE_INCDIR_OPENGL)
        !qtConfig(dynamicgl): CMAKE_OPENGL_LIBS = $$cmakeProcessLibs($$QMAKE_LIBS_OPENGL)
        !isEmpty(QMAKE_LIBDIR_OPENGL): CMAKE_OPENGL_LIBDIR = $$cmakePortablePaths($$QMAKE_LIBDIR_OPENGL)
        CMAKE_GL_HEADER_NAME = GL/gl.h
        mac: CMAKE_GL_HEADER_NAME = gl.h
        CMAKE_QT_OPENGL_IMPLEMENTATION = GL
    }
}

qtConfig(egl): CMAKE_EGL_INCDIRS = $$cmakePortablePaths($$QMAKE_INCDIR_EGL)

QMAKE_DYNAMIC_LIST_FILE = $$PWD/QtGui.dynlist

TRACEPOINT_PROVIDER = $$PWD/qtgui.tracepoints
CONFIG += qt_tracepoints metatypes install_metatypes

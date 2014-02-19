/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QByteArray>
#include <QVector>
#include <QCoreApplication>
#include <QLoggingCategory>
#include <qt_windows.h>
// Must not include QOpenGLFunctions or anything that pulls in qopengl.h.
// Otherwise we end up with errors about inconsistent linkage.
#include <GL/gl.h>
#include <EGL/egl.h>

#if defined(QT_OPENGL_ES) || defined(QT_OPENGL_ES_2)
#  error "Proxy GL is not compatible with static ES builds"
#endif

// This should not be an issue with the compilers used on Windows, but just in case:
#ifndef Q_COMPILER_VARIADIC_MACROS
#  error "Proxy GL requires variadic macro support"
#endif

// Disable inconsistent dll linkage warnings. gl.h and egl.h are included and these mark
// the egl and (w)gl functions as imported. We will mark them as exported.
#if defined(Q_CC_MSVC)
#  pragma warning(disable : 4273)
#elif defined(Q_CC_MINGW)
#  pragma GCC diagnostic ignored "-Wattributes"
#endif

#ifdef Q_OS_WIN64
typedef signed   long long int khronos_intptr_t;
typedef signed   long long int khronos_ssize_t;
#else
typedef signed   long  int     khronos_intptr_t;
typedef signed   long  int     khronos_ssize_t;
#endif

typedef char             GLchar;
typedef khronos_intptr_t GLintptr;
typedef khronos_ssize_t  GLsizeiptr;

Q_LOGGING_CATEGORY(qglLc, "qt.gui.openglproxy")

class QAbstractWindowsOpenGL
{
public:
    QAbstractWindowsOpenGL();
    virtual ~QAbstractWindowsOpenGL() { }

    enum LibType { // must match QOpenGLFunctions::PlatformGLType
        DesktopGL = 0,
        GLES2
    };

    LibType libraryType() const { return m_libraryType; }
    HMODULE libraryHandle() const { return m_lib; }
    bool functionsReady() const { return m_loaded; }

    // WGL
    BOOL (WINAPI * CopyContext)(HGLRC src, HGLRC dst, UINT mask);
    HGLRC (WINAPI * CreateContext)(HDC dc);
    HGLRC (WINAPI * CreateLayerContext)(HDC dc, int plane);
    BOOL (WINAPI * DeleteContext)(HGLRC context);
    HGLRC (WINAPI * GetCurrentContext)();
    HDC (WINAPI * GetCurrentDC)();
    PROC (WINAPI * GetProcAddress)(LPCSTR name);
    BOOL (WINAPI * MakeCurrent)(HDC dc, HGLRC context);
    BOOL (WINAPI * ShareLists)(HGLRC context1, HGLRC context2);
    BOOL (WINAPI * UseFontBitmapsW)(HDC dc, DWORD first, DWORD count, DWORD base);
    BOOL (WINAPI * UseFontOutlinesW)(HDC dc, DWORD first, DWORD count, DWORD base, FLOAT deviation,
                                     FLOAT extrusion, int format, LPGLYPHMETRICSFLOAT gmf);
    BOOL (WINAPI * DescribeLayerPlane)(HDC dc, int pixelFormat, int plane, UINT n,
                                       LPLAYERPLANEDESCRIPTOR planeDescriptor);
    int (WINAPI * SetLayerPaletteEntries)(HDC dc, int plane, int start, int entries,
                                          CONST COLORREF *colors);
    int (WINAPI * GetLayerPaletteEntries)(HDC dc, int plane, int start, int entries,
                                          COLORREF *color);
    BOOL (WINAPI * RealizeLayerPalette)(HDC dc, int plane, BOOL realize);
    BOOL (WINAPI * SwapLayerBuffers)(HDC dc, UINT planes);
    DWORD (WINAPI * SwapMultipleBuffers)(UINT n, CONST WGLSWAP *buffers);

    // EGL
    EGLint (EGLAPIENTRY * EGL_GetError)(void);
    EGLDisplay (EGLAPIENTRY * EGL_GetDisplay)(EGLNativeDisplayType display_id);
    EGLBoolean (EGLAPIENTRY * EGL_Initialize)(EGLDisplay dpy, EGLint *major, EGLint *minor);
    EGLBoolean (EGLAPIENTRY * EGL_Terminate)(EGLDisplay dpy);
    const char * (EGLAPIENTRY * EGL_QueryString)(EGLDisplay dpy, EGLint name);
    EGLBoolean (EGLAPIENTRY * EGL_GetConfigs)(EGLDisplay dpy, EGLConfig *configs,
                                              EGLint config_size, EGLint *num_config);
    EGLBoolean (EGLAPIENTRY * EGL_ChooseConfig)(EGLDisplay dpy, const EGLint *attrib_list,
                                                EGLConfig *configs, EGLint config_size,
                                                EGLint *num_config);
    EGLBoolean (EGLAPIENTRY * EGL_GetConfigAttrib)(EGLDisplay dpy, EGLConfig config,
                                                   EGLint attribute, EGLint *value);
    EGLSurface (EGLAPIENTRY * EGL_CreateWindowSurface)(EGLDisplay dpy, EGLConfig config,
                                                       EGLNativeWindowType win,
                                                       const EGLint *attrib_list);
    EGLSurface (EGLAPIENTRY * EGL_CreatePbufferSurface)(EGLDisplay dpy, EGLConfig config,
                                                        const EGLint *attrib_list);
    EGLSurface (EGLAPIENTRY * EGL_CreatePixmapSurface)(EGLDisplay dpy, EGLConfig config,
                                                       EGLNativePixmapType pixmap,
                                                       const EGLint *attrib_list);
    EGLBoolean (EGLAPIENTRY * EGL_DestroySurface)(EGLDisplay dpy, EGLSurface surface);
    EGLBoolean (EGLAPIENTRY * EGL_QuerySurface)(EGLDisplay dpy, EGLSurface surface,
                                                EGLint attribute, EGLint *value);
    EGLBoolean (EGLAPIENTRY * EGL_BindAPI)(EGLenum api);
    EGLenum (EGLAPIENTRY * EGL_QueryAPI)(void);
    EGLBoolean (EGLAPIENTRY * EGL_WaitClient)(void);
    EGLBoolean (EGLAPIENTRY * EGL_ReleaseThread)(void);
    EGLSurface (EGLAPIENTRY * EGL_CreatePbufferFromClientBuffer)(EGLDisplay dpy, EGLenum buftype,
                                                                 EGLClientBuffer buffer,
                                                                 EGLConfig config, const EGLint *attrib_list);
    EGLBoolean (EGLAPIENTRY * EGL_SurfaceAttrib)(EGLDisplay dpy, EGLSurface surface,
                                                 EGLint attribute, EGLint value);
    EGLBoolean (EGLAPIENTRY * EGL_BindTexImage)(EGLDisplay dpy, EGLSurface surface, EGLint buffer);
    EGLBoolean (EGLAPIENTRY * EGL_ReleaseTexImage)(EGLDisplay dpy, EGLSurface surface, EGLint buffer);
    EGLBoolean (EGLAPIENTRY * EGL_SwapInterval)(EGLDisplay dpy, EGLint interval);
    EGLContext (EGLAPIENTRY * EGL_CreateContext)(EGLDisplay dpy, EGLConfig config,
                                                 EGLContext share_context,
                                                 const EGLint *attrib_list);
    EGLBoolean (EGLAPIENTRY * EGL_DestroyContext)(EGLDisplay dpy, EGLContext ctx);
    EGLBoolean (EGLAPIENTRY * EGL_MakeCurrent)(EGLDisplay dpy, EGLSurface draw,
                                               EGLSurface read, EGLContext ctx);
    EGLContext (EGLAPIENTRY * EGL_GetCurrentContext)(void);
    EGLSurface (EGLAPIENTRY * EGL_GetCurrentSurface)(EGLint readdraw);
    EGLDisplay (EGLAPIENTRY * EGL_GetCurrentDisplay)(void);
    EGLBoolean (EGLAPIENTRY * EGL_QueryContext)(EGLDisplay dpy, EGLContext ctx,
                                                EGLint attribute, EGLint *value);
    EGLBoolean (EGLAPIENTRY * EGL_WaitGL)(void);
    EGLBoolean (EGLAPIENTRY * EGL_WaitNative)(EGLint engine);
    EGLBoolean (EGLAPIENTRY * EGL_SwapBuffers)(EGLDisplay dpy, EGLSurface surface);
    EGLBoolean (EGLAPIENTRY * EGL_CopyBuffers)(EGLDisplay dpy, EGLSurface surface,
                                               EGLNativePixmapType target);
    __eglMustCastToProperFunctionPointerType (EGLAPIENTRY * EGL_GetProcAddress)(const char *procname);

    // OpenGL 1.0
    void (APIENTRY * Viewport)(GLint x, GLint y, GLsizei width, GLsizei height);
    void (APIENTRY * DepthRange)(GLdouble nearVal, GLdouble farVal);
    GLboolean (APIENTRY * IsEnabled)(GLenum cap);
    void (APIENTRY * GetTexLevelParameteriv)(GLenum target, GLint level, GLenum pname, GLint *params);
    void (APIENTRY * GetTexLevelParameterfv)(GLenum target, GLint level, GLenum pname, GLfloat *params);
    void (APIENTRY * GetTexParameteriv)(GLenum target, GLenum pname, GLint *params);
    void (APIENTRY * GetTexParameterfv)(GLenum target, GLenum pname, GLfloat *params);
    void (APIENTRY * GetTexImage)(GLenum target, GLint level, GLenum format, GLenum type, GLvoid *pixels);
    const GLubyte * (APIENTRY * GetString)(GLenum name);
    void (APIENTRY * GetIntegerv)(GLenum pname, GLint *params);
    void (APIENTRY * GetFloatv)(GLenum pname, GLfloat *params);
    GLenum (APIENTRY * GetError)();
    void (APIENTRY * GetDoublev)(GLenum pname, GLdouble *params);
    void (APIENTRY * GetBooleanv)(GLenum pname, GLboolean *params);
    void (APIENTRY * ReadPixels)(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid *pixels);
    void (APIENTRY * ReadBuffer)(GLenum mode);
    void (APIENTRY * PixelStorei)(GLenum pname, GLint param);
    void (APIENTRY * PixelStoref)(GLenum pname, GLfloat param);
    void (APIENTRY * DepthFunc)(GLenum func);
    void (APIENTRY * StencilOp)(GLenum fail, GLenum zfail, GLenum zpass);
    void (APIENTRY * StencilFunc)(GLenum func, GLint ref, GLuint mask);
    void (APIENTRY * LogicOp)(GLenum opcode);
    void (APIENTRY * BlendFunc)(GLenum sfactor, GLenum dfactor);
    void (APIENTRY * Flush)();
    void (APIENTRY * Finish)();
    void (APIENTRY * Enable)(GLenum cap);
    void (APIENTRY * Disable)(GLenum cap);
    void (APIENTRY * DepthMask)(GLboolean flag);
    void (APIENTRY * ColorMask)(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha);
    void (APIENTRY * StencilMask)(GLuint mask);
    void (APIENTRY * ClearDepth)(GLdouble depth);
    void (APIENTRY * ClearStencil)(GLint s);
    void (APIENTRY * ClearColor)(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
    void (APIENTRY * Clear)(GLbitfield mask);
    void (APIENTRY * DrawBuffer)(GLenum mode);
    void (APIENTRY * TexImage2D)(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *pixels);
    void (APIENTRY * TexImage1D)(GLenum target, GLint level, GLint internalformat, GLsizei width, GLint border, GLenum format, GLenum type, const GLvoid *pixels);
    void (APIENTRY * TexParameteriv)(GLenum target, GLenum pname, const GLint *params);
    void (APIENTRY * TexParameteri)(GLenum target, GLenum pname, GLint param);
    void (APIENTRY * TexParameterfv)(GLenum target, GLenum pname, const GLfloat *params);
    void (APIENTRY * TexParameterf)(GLenum target, GLenum pname, GLfloat param);
    void (APIENTRY * Scissor)(GLint x, GLint y, GLsizei width, GLsizei height);
    void (APIENTRY * PolygonMode)(GLenum face, GLenum mode);
    void (APIENTRY * PointSize)(GLfloat size);
    void (APIENTRY * LineWidth)(GLfloat width);
    void (APIENTRY * Hint)(GLenum target, GLenum mode);
    void (APIENTRY * FrontFace)(GLenum mode);
    void (APIENTRY * CullFace)(GLenum mode);

    void (APIENTRY * Translatef)(GLfloat x, GLfloat y, GLfloat z);
    void (APIENTRY * Translated)(GLdouble x, GLdouble y, GLdouble z);
    void (APIENTRY * Scalef)(GLfloat x, GLfloat y, GLfloat z);
    void (APIENTRY * Scaled)(GLdouble x, GLdouble y, GLdouble z);
    void (APIENTRY * Rotatef)(GLfloat angle, GLfloat x, GLfloat y, GLfloat z);
    void (APIENTRY * Rotated)(GLdouble angle, GLdouble x, GLdouble y, GLdouble z);
    void (APIENTRY * PushMatrix)();
    void (APIENTRY * PopMatrix)();
    void (APIENTRY * Ortho)(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar);
    void (APIENTRY * MultMatrixd)(const GLdouble *m);
    void (APIENTRY * MultMatrixf)(const GLfloat *m);
    void (APIENTRY * MatrixMode)(GLenum mode);
    void (APIENTRY * LoadMatrixd)(const GLdouble *m);
    void (APIENTRY * LoadMatrixf)(const GLfloat *m);
    void (APIENTRY * LoadIdentity)();
    void (APIENTRY * Frustum)(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar);
    GLboolean (APIENTRY * IsList)(GLuint list);
    void (APIENTRY * GetTexGeniv)(GLenum coord, GLenum pname, GLint *params);
    void (APIENTRY * GetTexGenfv)(GLenum coord, GLenum pname, GLfloat *params);
    void (APIENTRY * GetTexGendv)(GLenum coord, GLenum pname, GLdouble *params);
    void (APIENTRY * GetTexEnviv)(GLenum target, GLenum pname, GLint *params);
    void (APIENTRY * GetTexEnvfv)(GLenum target, GLenum pname, GLfloat *params);
    void (APIENTRY * GetPolygonStipple)(GLubyte *mask);
    void (APIENTRY * GetPixelMapusv)(GLenum map, GLushort *values);
    void (APIENTRY * GetPixelMapuiv)(GLenum map, GLuint *values);
    void (APIENTRY * GetPixelMapfv)(GLenum map, GLfloat *values);
    void (APIENTRY * GetMaterialiv)(GLenum face, GLenum pname, GLint *params);
    void (APIENTRY * GetMaterialfv)(GLenum face, GLenum pname, GLfloat *params);
    void (APIENTRY * GetMapiv)(GLenum target, GLenum query, GLint *v);
    void (APIENTRY * GetMapfv)(GLenum target, GLenum query, GLfloat *v);
    void (APIENTRY * GetMapdv)(GLenum target, GLenum query, GLdouble *v);
    void (APIENTRY * GetLightiv)(GLenum light, GLenum pname, GLint *params);
    void (APIENTRY * GetLightfv)(GLenum light, GLenum pname, GLfloat *params);
    void (APIENTRY * GetClipPlane)(GLenum plane, GLdouble *equation);
    void (APIENTRY * DrawPixels)(GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels);
    void (APIENTRY * CopyPixels)(GLint x, GLint y, GLsizei width, GLsizei height, GLenum type);
    void (APIENTRY * PixelMapusv)(GLenum map, GLint mapsize, const GLushort *values);
    void (APIENTRY * PixelMapuiv)(GLenum map, GLint mapsize, const GLuint *values);
    void (APIENTRY * PixelMapfv)(GLenum map, GLint mapsize, const GLfloat *values);
    void (APIENTRY * PixelTransferi)(GLenum pname, GLint param);
    void (APIENTRY * PixelTransferf)(GLenum pname, GLfloat param);
    void (APIENTRY * PixelZoom)(GLfloat xfactor, GLfloat yfactor);
    void (APIENTRY * AlphaFunc)(GLenum func, GLfloat ref);
    void (APIENTRY * EvalPoint2)(GLint i, GLint j);
    void (APIENTRY * EvalMesh2)(GLenum mode, GLint i1, GLint i2, GLint j1, GLint j2);
    void (APIENTRY * EvalPoint1)(GLint i);
    void (APIENTRY * EvalMesh1)(GLenum mode, GLint i1, GLint i2);
    void (APIENTRY * EvalCoord2fv)(const GLfloat *u);
    void (APIENTRY * EvalCoord2f)(GLfloat u, GLfloat v);
    void (APIENTRY * EvalCoord2dv)(const GLdouble *u);
    void (APIENTRY * EvalCoord2d)(GLdouble u, GLdouble v);
    void (APIENTRY * EvalCoord1fv)(const GLfloat *u);
    void (APIENTRY * EvalCoord1f)(GLfloat u);
    void (APIENTRY * EvalCoord1dv)(const GLdouble *u);
    void (APIENTRY * EvalCoord1d)(GLdouble u);
    void (APIENTRY * MapGrid2f)(GLint un, GLfloat u1, GLfloat u2, GLint vn, GLfloat v1, GLfloat v2);
    void (APIENTRY * MapGrid2d)(GLint un, GLdouble u1, GLdouble u2, GLint vn, GLdouble v1, GLdouble v2);
    void (APIENTRY * MapGrid1f)(GLint un, GLfloat u1, GLfloat u2);
    void (APIENTRY * MapGrid1d)(GLint un, GLdouble u1, GLdouble u2);
    void (APIENTRY * Map2f)(GLenum target, GLfloat u1, GLfloat u2, GLint ustride, GLint uorder, GLfloat v1, GLfloat v2, GLint vstride, GLint vorder, const GLfloat *points);
    void (APIENTRY * Map2d)(GLenum target, GLdouble u1, GLdouble u2, GLint ustride, GLint uorder, GLdouble v1, GLdouble v2, GLint vstride, GLint vorder, const GLdouble *points);
    void (APIENTRY * Map1f)(GLenum target, GLfloat u1, GLfloat u2, GLint stride, GLint order, const GLfloat *points);
    void (APIENTRY * Map1d)(GLenum target, GLdouble u1, GLdouble u2, GLint stride, GLint order, const GLdouble *points);
    void (APIENTRY * PushAttrib)(GLbitfield mask);
    void (APIENTRY * PopAttrib)();
    void (APIENTRY * Accum)(GLenum op, GLfloat value);
    void (APIENTRY * IndexMask)(GLuint mask);
    void (APIENTRY * ClearIndex)(GLfloat c);
    void (APIENTRY * ClearAccum)(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
    void (APIENTRY * PushName)(GLuint name);
    void (APIENTRY * PopName)();
    void (APIENTRY * PassThrough)(GLfloat token);
    void (APIENTRY * LoadName)(GLuint name);
    void (APIENTRY * InitNames)();
    GLint (APIENTRY * RenderMode)(GLenum mode);
    void (APIENTRY * SelectBuffer)(GLsizei size, GLuint *buffer);
    void (APIENTRY * FeedbackBuffer)(GLsizei size, GLenum type, GLfloat *buffer);
    void (APIENTRY * TexGeniv)(GLenum coord, GLenum pname, const GLint *params);
    void (APIENTRY * TexGeni)(GLenum coord, GLenum pname, GLint param);
    void (APIENTRY * TexGenfv)(GLenum coord, GLenum pname, const GLfloat *params);
    void (APIENTRY * TexGenf)(GLenum coord, GLenum pname, GLfloat param);
    void (APIENTRY * TexGendv)(GLenum coord, GLenum pname, const GLdouble *params);
    void (APIENTRY * TexGend)(GLenum coord, GLenum pname, GLdouble param);
    void (APIENTRY * TexEnviv)(GLenum target, GLenum pname, const GLint *params);
    void (APIENTRY * TexEnvi)(GLenum target, GLenum pname, GLint param);
    void (APIENTRY * TexEnvfv)(GLenum target, GLenum pname, const GLfloat *params);
    void (APIENTRY * TexEnvf)(GLenum target, GLenum pname, GLfloat param);
    void (APIENTRY * ShadeModel)(GLenum mode);
    void (APIENTRY * PolygonStipple)(const GLubyte *mask);
    void (APIENTRY * Materialiv)(GLenum face, GLenum pname, const GLint *params);
    void (APIENTRY * Materiali)(GLenum face, GLenum pname, GLint param);
    void (APIENTRY * Materialfv)(GLenum face, GLenum pname, const GLfloat *params);
    void (APIENTRY * Materialf)(GLenum face, GLenum pname, GLfloat param);
    void (APIENTRY * LineStipple)(GLint factor, GLushort pattern);
    void (APIENTRY * LightModeliv)(GLenum pname, const GLint *params);
    void (APIENTRY * LightModeli)(GLenum pname, GLint param);
    void (APIENTRY * LightModelfv)(GLenum pname, const GLfloat *params);
    void (APIENTRY * LightModelf)(GLenum pname, GLfloat param);
    void (APIENTRY * Lightiv)(GLenum light, GLenum pname, const GLint *params);
    void (APIENTRY * Lighti)(GLenum light, GLenum pname, GLint param);
    void (APIENTRY * Lightfv)(GLenum light, GLenum pname, const GLfloat *params);
    void (APIENTRY * Lightf)(GLenum light, GLenum pname, GLfloat param);
    void (APIENTRY * Fogiv)(GLenum pname, const GLint *params);
    void (APIENTRY * Fogi)(GLenum pname, GLint param);
    void (APIENTRY * Fogfv)(GLenum pname, const GLfloat *params);
    void (APIENTRY * Fogf)(GLenum pname, GLfloat param);
    void (APIENTRY * ColorMaterial)(GLenum face, GLenum mode);
    void (APIENTRY * ClipPlane)(GLenum plane, const GLdouble *equation);
    void (APIENTRY * Vertex4sv)(const GLshort *v);
    void (APIENTRY * Vertex4s)(GLshort x, GLshort y, GLshort z, GLshort w);
    void (APIENTRY * Vertex4iv)(const GLint *v);
    void (APIENTRY * Vertex4i)(GLint x, GLint y, GLint z, GLint w);
    void (APIENTRY * Vertex4fv)(const GLfloat *v);
    void (APIENTRY * Vertex4f)(GLfloat x, GLfloat y, GLfloat z, GLfloat w);
    void (APIENTRY * Vertex4dv)(const GLdouble *v);
    void (APIENTRY * Vertex4d)(GLdouble x, GLdouble y, GLdouble z, GLdouble w);
    void (APIENTRY * Vertex3sv)(const GLshort *v);
    void (APIENTRY * Vertex3s)(GLshort x, GLshort y, GLshort z);
    void (APIENTRY * Vertex3iv)(const GLint *v);
    void (APIENTRY * Vertex3i)(GLint x, GLint y, GLint z);
    void (APIENTRY * Vertex3fv)(const GLfloat *v);
    void (APIENTRY * Vertex3f)(GLfloat x, GLfloat y, GLfloat z);
    void (APIENTRY * Vertex3dv)(const GLdouble *v);
    void (APIENTRY * Vertex3d)(GLdouble x, GLdouble y, GLdouble z);
    void (APIENTRY * Vertex2sv)(const GLshort *v);
    void (APIENTRY * Vertex2s)(GLshort x, GLshort y);
    void (APIENTRY * Vertex2iv)(const GLint *v);
    void (APIENTRY * Vertex2i)(GLint x, GLint y);
    void (APIENTRY * Vertex2fv)(const GLfloat *v);
    void (APIENTRY * Vertex2f)(GLfloat x, GLfloat y);
    void (APIENTRY * Vertex2dv)(const GLdouble *v);
    void (APIENTRY * Vertex2d)(GLdouble x, GLdouble y);
    void (APIENTRY * TexCoord4sv)(const GLshort *v);
    void (APIENTRY * TexCoord4s)(GLshort s, GLshort t, GLshort r, GLshort q);
    void (APIENTRY * TexCoord4iv)(const GLint *v);
    void (APIENTRY * TexCoord4i)(GLint s, GLint t, GLint r, GLint q);
    void (APIENTRY * TexCoord4fv)(const GLfloat *v);
    void (APIENTRY * TexCoord4f)(GLfloat s, GLfloat t, GLfloat r, GLfloat q);
    void (APIENTRY * TexCoord4dv)(const GLdouble *v);
    void (APIENTRY * TexCoord4d)(GLdouble s, GLdouble t, GLdouble r, GLdouble q);
    void (APIENTRY * TexCoord3sv)(const GLshort *v);
    void (APIENTRY * TexCoord3s)(GLshort s, GLshort t, GLshort r);
    void (APIENTRY * TexCoord3iv)(const GLint *v);
    void (APIENTRY * TexCoord3i)(GLint s, GLint t, GLint r);
    void (APIENTRY * TexCoord3fv)(const GLfloat *v);
    void (APIENTRY * TexCoord3f)(GLfloat s, GLfloat t, GLfloat r);
    void (APIENTRY * TexCoord3dv)(const GLdouble *v);
    void (APIENTRY * TexCoord3d)(GLdouble s, GLdouble t, GLdouble r);
    void (APIENTRY * TexCoord2sv)(const GLshort *v);
    void (APIENTRY * TexCoord2s)(GLshort s, GLshort t);
    void (APIENTRY * TexCoord2iv)(const GLint *v);
    void (APIENTRY * TexCoord2i)(GLint s, GLint t);
    void (APIENTRY * TexCoord2fv)(const GLfloat *v);
    void (APIENTRY * TexCoord2f)(GLfloat s, GLfloat t);
    void (APIENTRY * TexCoord2dv)(const GLdouble *v);
    void (APIENTRY * TexCoord2d)(GLdouble s, GLdouble t);
    void (APIENTRY * TexCoord1sv)(const GLshort *v);
    void (APIENTRY * TexCoord1s)(GLshort s);
    void (APIENTRY * TexCoord1iv)(const GLint *v);
    void (APIENTRY * TexCoord1i)(GLint s);
    void (APIENTRY * TexCoord1fv)(const GLfloat *v);
    void (APIENTRY * TexCoord1f)(GLfloat s);
    void (APIENTRY * TexCoord1dv)(const GLdouble *v);
    void (APIENTRY * TexCoord1d)(GLdouble s);
    void (APIENTRY * Rectsv)(const GLshort *v1, const GLshort *v2);
    void (APIENTRY * Rects)(GLshort x1, GLshort y1, GLshort x2, GLshort y2);
    void (APIENTRY * Rectiv)(const GLint *v1, const GLint *v2);
    void (APIENTRY * Recti)(GLint x1, GLint y1, GLint x2, GLint y2);
    void (APIENTRY * Rectfv)(const GLfloat *v1, const GLfloat *v2);
    void (APIENTRY * Rectf)(GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2);
    void (APIENTRY * Rectdv)(const GLdouble *v1, const GLdouble *v2);
    void (APIENTRY * Rectd)(GLdouble x1, GLdouble y1, GLdouble x2, GLdouble y2);
    void (APIENTRY * RasterPos4sv)(const GLshort *v);
    void (APIENTRY * RasterPos4s)(GLshort x, GLshort y, GLshort z, GLshort w);
    void (APIENTRY * RasterPos4iv)(const GLint *v);
    void (APIENTRY * RasterPos4i)(GLint x, GLint y, GLint z, GLint w);
    void (APIENTRY * RasterPos4fv)(const GLfloat *v);
    void (APIENTRY * RasterPos4f)(GLfloat x, GLfloat y, GLfloat z, GLfloat w);
    void (APIENTRY * RasterPos4dv)(const GLdouble *v);
    void (APIENTRY * RasterPos4d)(GLdouble x, GLdouble y, GLdouble z, GLdouble w);
    void (APIENTRY * RasterPos3sv)(const GLshort *v);
    void (APIENTRY * RasterPos3s)(GLshort x, GLshort y, GLshort z);
    void (APIENTRY * RasterPos3iv)(const GLint *v);
    void (APIENTRY * RasterPos3i)(GLint x, GLint y, GLint z);
    void (APIENTRY * RasterPos3fv)(const GLfloat *v);
    void (APIENTRY * RasterPos3f)(GLfloat x, GLfloat y, GLfloat z);
    void (APIENTRY * RasterPos3dv)(const GLdouble *v);
    void (APIENTRY * RasterPos3d)(GLdouble x, GLdouble y, GLdouble z);
    void (APIENTRY * RasterPos2sv)(const GLshort *v);
    void (APIENTRY * RasterPos2s)(GLshort x, GLshort y);
    void (APIENTRY * RasterPos2iv)(const GLint *v);
    void (APIENTRY * RasterPos2i)(GLint x, GLint y);
    void (APIENTRY * RasterPos2fv)(const GLfloat *v);
    void (APIENTRY * RasterPos2f)(GLfloat x, GLfloat y);
    void (APIENTRY * RasterPos2dv)(const GLdouble *v);
    void (APIENTRY * RasterPos2d)(GLdouble x, GLdouble y);
    void (APIENTRY * Normal3sv)(const GLshort *v);
    void (APIENTRY * Normal3s)(GLshort nx, GLshort ny, GLshort nz);
    void (APIENTRY * Normal3iv)(const GLint *v);
    void (APIENTRY * Normal3i)(GLint nx, GLint ny, GLint nz);
    void (APIENTRY * Normal3fv)(const GLfloat *v);
    void (APIENTRY * Normal3f)(GLfloat nx, GLfloat ny, GLfloat nz);
    void (APIENTRY * Normal3dv)(const GLdouble *v);
    void (APIENTRY * Normal3d)(GLdouble nx, GLdouble ny, GLdouble nz);
    void (APIENTRY * Normal3bv)(const GLbyte *v);
    void (APIENTRY * Normal3b)(GLbyte nx, GLbyte ny, GLbyte nz);
    void (APIENTRY * Indexsv)(const GLshort *c);
    void (APIENTRY * Indexs)(GLshort c);
    void (APIENTRY * Indexiv)(const GLint *c);
    void (APIENTRY * Indexi)(GLint c);
    void (APIENTRY * Indexfv)(const GLfloat *c);
    void (APIENTRY * Indexf)(GLfloat c);
    void (APIENTRY * Indexdv)(const GLdouble *c);
    void (APIENTRY * Indexd)(GLdouble c);
    void (APIENTRY * End)();
    void (APIENTRY * EdgeFlagv)(const GLboolean *flag);
    void (APIENTRY * EdgeFlag)(GLboolean flag);
    void (APIENTRY * Color4usv)(const GLushort *v);
    void (APIENTRY * Color4us)(GLushort red, GLushort green, GLushort blue, GLushort alpha);
    void (APIENTRY * Color4uiv)(const GLuint *v);
    void (APIENTRY * Color4ui)(GLuint red, GLuint green, GLuint blue, GLuint alpha);
    void (APIENTRY * Color4ubv)(const GLubyte *v);
    void (APIENTRY * Color4ub)(GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha);
    void (APIENTRY * Color4sv)(const GLshort *v);
    void (APIENTRY * Color4s)(GLshort red, GLshort green, GLshort blue, GLshort alpha);
    void (APIENTRY * Color4iv)(const GLint *v);
    void (APIENTRY * Color4i)(GLint red, GLint green, GLint blue, GLint alpha);
    void (APIENTRY * Color4fv)(const GLfloat *v);
    void (APIENTRY * Color4f)(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
    void (APIENTRY * Color4dv)(const GLdouble *v);
    void (APIENTRY * Color4d)(GLdouble red, GLdouble green, GLdouble blue, GLdouble alpha);
    void (APIENTRY * Color4bv)(const GLbyte *v);
    void (APIENTRY * Color4b)(GLbyte red, GLbyte green, GLbyte blue, GLbyte alpha);
    void (APIENTRY * Color3usv)(const GLushort *v);
    void (APIENTRY * Color3us)(GLushort red, GLushort green, GLushort blue);
    void (APIENTRY * Color3uiv)(const GLuint *v);
    void (APIENTRY * Color3ui)(GLuint red, GLuint green, GLuint blue);
    void (APIENTRY * Color3ubv)(const GLubyte *v);
    void (APIENTRY * Color3ub)(GLubyte red, GLubyte green, GLubyte blue);
    void (APIENTRY * Color3sv)(const GLshort *v);
    void (APIENTRY * Color3s)(GLshort red, GLshort green, GLshort blue);
    void (APIENTRY * Color3iv)(const GLint *v);
    void (APIENTRY * Color3i)(GLint red, GLint green, GLint blue);
    void (APIENTRY * Color3fv)(const GLfloat *v);
    void (APIENTRY * Color3f)(GLfloat red, GLfloat green, GLfloat blue);
    void (APIENTRY * Color3dv)(const GLdouble *v);
    void (APIENTRY * Color3d)(GLdouble red, GLdouble green, GLdouble blue);
    void (APIENTRY * Color3bv)(const GLbyte *v);
    void (APIENTRY * Color3b)(GLbyte red, GLbyte green, GLbyte blue);
    void (APIENTRY * Bitmap)(GLsizei width, GLsizei height, GLfloat xorig, GLfloat yorig, GLfloat xmove, GLfloat ymove, const GLubyte *bitmap);
    void (APIENTRY * Begin)(GLenum mode);
    void (APIENTRY * ListBase)(GLuint base);
    GLuint (APIENTRY * GenLists)(GLsizei range);
    void (APIENTRY * DeleteLists)(GLuint list, GLsizei range);
    void (APIENTRY * CallLists)(GLsizei n, GLenum type, const GLvoid *lists);
    void (APIENTRY * CallList)(GLuint list);
    void (APIENTRY * EndList)();
    void (APIENTRY * NewList)(GLuint list, GLenum mode);

    // OpenGL 1.1
    void (APIENTRY * Indexubv)(const GLubyte *c);
    void (APIENTRY * Indexub)(GLubyte c);
    GLboolean (APIENTRY * IsTexture)(GLuint texture);
    void (APIENTRY * GenTextures)(GLsizei n, GLuint *textures);
    void (APIENTRY * DeleteTextures)(GLsizei n, const GLuint *textures);
    void (APIENTRY * BindTexture)(GLenum target, GLuint texture);
    void (APIENTRY * TexSubImage2D)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels);
    void (APIENTRY * TexSubImage1D)(GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const GLvoid *pixels);
    void (APIENTRY * CopyTexSubImage2D)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height);
    void (APIENTRY * CopyTexSubImage1D)(GLenum target, GLint level, GLint xoffset, GLint x, GLint y, GLsizei width);
    void (APIENTRY * CopyTexImage2D)(GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border);
    void (APIENTRY * CopyTexImage1D)(GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLint border);
    void (APIENTRY * PolygonOffset)(GLfloat factor, GLfloat units);
    void (APIENTRY * GetPointerv)(GLenum pname, GLvoid* *params);
    void (APIENTRY * DrawElements)(GLenum mode, GLsizei count, GLenum type, const GLvoid *indices);
    void (APIENTRY * DrawArrays)(GLenum mode, GLint first, GLsizei count);

    void (APIENTRY * PushClientAttrib)(GLbitfield mask);
    void (APIENTRY * PopClientAttrib)();
    void (APIENTRY * PrioritizeTextures)(GLsizei n, const GLuint *textures, const GLfloat *priorities);
    GLboolean (APIENTRY * AreTexturesResident)(GLsizei n, const GLuint *textures, GLboolean *residences);
    void (APIENTRY * VertexPointer)(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
    void (APIENTRY * TexCoordPointer)(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
    void (APIENTRY * NormalPointer)(GLenum type, GLsizei stride, const GLvoid *pointer);
    void (APIENTRY * InterleavedArrays)(GLenum format, GLsizei stride, const GLvoid *pointer);
    void (APIENTRY * IndexPointer)(GLenum type, GLsizei stride, const GLvoid *pointer);
    void (APIENTRY * EnableClientState)(GLenum array);
    void (APIENTRY * EdgeFlagPointer)(GLsizei stride, const GLvoid *pointer);
    void (APIENTRY * DisableClientState)(GLenum array);
    void (APIENTRY * ColorPointer)(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
    void (APIENTRY * ArrayElement)(GLint i);

    // OpenGL ES 2.0
    void (APIENTRY * ActiveTexture)(GLenum texture);
    void (APIENTRY * AttachShader)(GLuint program, GLuint shader);
    void (APIENTRY * BindAttribLocation)(GLuint program, GLuint index, const GLchar* name);
    void (APIENTRY * BindBuffer)(GLenum target, GLuint buffer);
    void (APIENTRY * BindFramebuffer)(GLenum target, GLuint framebuffer);
    void (APIENTRY * BindRenderbuffer)(GLenum target, GLuint renderbuffer);
    void (APIENTRY * BlendColor)(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha);
    void (APIENTRY * BlendEquation)(GLenum mode);
    void (APIENTRY * BlendEquationSeparate)(GLenum modeRGB, GLenum modeAlpha);
    void (APIENTRY * BlendFuncSeparate)(GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha);
    void (APIENTRY * BufferData)(GLenum target, GLsizeiptr size, const GLvoid* data, GLenum usage);
    void (APIENTRY * BufferSubData)(GLenum target, GLintptr offset, GLsizeiptr size, const GLvoid* data);
    GLenum (APIENTRY * CheckFramebufferStatus)(GLenum target);
    void (APIENTRY * ClearDepthf)(GLclampf depth);
    void (APIENTRY * CompileShader)(GLuint shader);
    void (APIENTRY * CompressedTexImage2D)(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const GLvoid* data);
    void (APIENTRY * CompressedTexSubImage2D)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const GLvoid* data);
    GLuint (APIENTRY * CreateProgram)(void);
    GLuint (APIENTRY * CreateShader)(GLenum type);
    void (APIENTRY * DeleteBuffers)(GLsizei n, const GLuint* buffers);
    void (APIENTRY * DeleteFramebuffers)(GLsizei n, const GLuint* framebuffers);
    void (APIENTRY * DeleteProgram)(GLuint program);
    void (APIENTRY * DeleteRenderbuffers)(GLsizei n, const GLuint* renderbuffers);
    void (APIENTRY * DeleteShader)(GLuint shader);
    void (APIENTRY * DepthRangef)(GLclampf zNear, GLclampf zFar);
    void (APIENTRY * DetachShader)(GLuint program, GLuint shader);
    void (APIENTRY * DisableVertexAttribArray)(GLuint index);
    void (APIENTRY * EnableVertexAttribArray)(GLuint index);
    void (APIENTRY * FramebufferRenderbuffer)(GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer);
    void (APIENTRY * FramebufferTexture2D)(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level);
    void (APIENTRY * GenBuffers)(GLsizei n, GLuint* buffers);
    void (APIENTRY * GenerateMipmap)(GLenum target);
    void (APIENTRY * GenFramebuffers)(GLsizei n, GLuint* framebuffers);
    void (APIENTRY * GenRenderbuffers)(GLsizei n, GLuint* renderbuffers);
    void (APIENTRY * GetActiveAttrib)(GLuint program, GLuint index, GLsizei bufsize, GLsizei* length, GLint* size, GLenum* type, GLchar* name);
    void (APIENTRY * GetActiveUniform)(GLuint program, GLuint index, GLsizei bufsize, GLsizei* length, GLint* size, GLenum* type, GLchar* name);
    void (APIENTRY * GetAttachedShaders)(GLuint program, GLsizei maxcount, GLsizei* count, GLuint* shaders);
    int (APIENTRY * GetAttribLocation)(GLuint program, const GLchar* name);
    void (APIENTRY * GetBufferParameteriv)(GLenum target, GLenum pname, GLint* params);
    void (APIENTRY * GetFramebufferAttachmentParameteriv)(GLenum target, GLenum attachment, GLenum pname, GLint* params);
    void (APIENTRY * GetProgramiv)(GLuint program, GLenum pname, GLint* params);
    void (APIENTRY * GetProgramInfoLog)(GLuint program, GLsizei bufsize, GLsizei* length, GLchar* infolog);
    void (APIENTRY * GetRenderbufferParameteriv)(GLenum target, GLenum pname, GLint* params);
    void (APIENTRY * GetShaderiv)(GLuint shader, GLenum pname, GLint* params);
    void (APIENTRY * GetShaderInfoLog)(GLuint shader, GLsizei bufsize, GLsizei* length, GLchar* infolog);
    void (APIENTRY * GetShaderPrecisionFormat)(GLenum shadertype, GLenum precisiontype, GLint* range, GLint* precision);
    void (APIENTRY * GetShaderSource)(GLuint shader, GLsizei bufsize, GLsizei* length, GLchar* source);
    void (APIENTRY * GetUniformfv)(GLuint program, GLint location, GLfloat* params);
    void (APIENTRY * GetUniformiv)(GLuint program, GLint location, GLint* params);
    int (APIENTRY * GetUniformLocation)(GLuint program, const GLchar* name);
    void (APIENTRY * GetVertexAttribfv)(GLuint index, GLenum pname, GLfloat* params);
    void (APIENTRY * GetVertexAttribiv)(GLuint index, GLenum pname, GLint* params);
    void (APIENTRY * GetVertexAttribPointerv)(GLuint index, GLenum pname, GLvoid** pointer);
    GLboolean (APIENTRY * IsBuffer)(GLuint buffer);
    GLboolean (APIENTRY * IsFramebuffer)(GLuint framebuffer);
    GLboolean (APIENTRY * IsProgram)(GLuint program);
    GLboolean (APIENTRY * IsRenderbuffer)(GLuint renderbuffer);
    GLboolean (APIENTRY * IsShader)(GLuint shader);
    void (APIENTRY * LinkProgram)(GLuint program);
    void (APIENTRY * ReleaseShaderCompiler)(void);
    void (APIENTRY * RenderbufferStorage)(GLenum target, GLenum internalformat, GLsizei width, GLsizei height);
    void (APIENTRY * SampleCoverage)(GLclampf value, GLboolean invert);
    void (APIENTRY * ShaderBinary)(GLsizei n, const GLuint* shaders, GLenum binaryformat, const GLvoid* binary, GLsizei length);
    void (APIENTRY * ShaderSource)(GLuint shader, GLsizei count, const GLchar* *string, const GLint* length);
    void (APIENTRY * StencilFuncSeparate)(GLenum face, GLenum func, GLint ref, GLuint mask);
    void (APIENTRY * StencilMaskSeparate)(GLenum face, GLuint mask);
    void (APIENTRY * StencilOpSeparate)(GLenum face, GLenum fail, GLenum zfail, GLenum zpass);
    void (APIENTRY * Uniform1f)(GLint location, GLfloat x);
    void (APIENTRY * Uniform1fv)(GLint location, GLsizei count, const GLfloat* v);
    void (APIENTRY * Uniform1i)(GLint location, GLint x);
    void (APIENTRY * Uniform1iv)(GLint location, GLsizei count, const GLint* v);
    void (APIENTRY * Uniform2f)(GLint location, GLfloat x, GLfloat y);
    void (APIENTRY * Uniform2fv)(GLint location, GLsizei count, const GLfloat* v);
    void (APIENTRY * Uniform2i)(GLint location, GLint x, GLint y);
    void (APIENTRY * Uniform2iv)(GLint location, GLsizei count, const GLint* v);
    void (APIENTRY * Uniform3f)(GLint location, GLfloat x, GLfloat y, GLfloat z);
    void (APIENTRY * Uniform3fv)(GLint location, GLsizei count, const GLfloat* v);
    void (APIENTRY * Uniform3i)(GLint location, GLint x, GLint y, GLint z);
    void (APIENTRY * Uniform3iv)(GLint location, GLsizei count, const GLint* v);
    void (APIENTRY * Uniform4f)(GLint location, GLfloat x, GLfloat y, GLfloat z, GLfloat w);
    void (APIENTRY * Uniform4fv)(GLint location, GLsizei count, const GLfloat* v);
    void (APIENTRY * Uniform4i)(GLint location, GLint x, GLint y, GLint z, GLint w);
    void (APIENTRY * Uniform4iv)(GLint location, GLsizei count, const GLint* v);
    void (APIENTRY * UniformMatrix2fv)(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value);
    void (APIENTRY * UniformMatrix3fv)(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value);
    void (APIENTRY * UniformMatrix4fv)(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value);
    void (APIENTRY * UseProgram)(GLuint program);
    void (APIENTRY * ValidateProgram)(GLuint program);
    void (APIENTRY * VertexAttrib1f)(GLuint indx, GLfloat x);
    void (APIENTRY * VertexAttrib1fv)(GLuint indx, const GLfloat* values);
    void (APIENTRY * VertexAttrib2f)(GLuint indx, GLfloat x, GLfloat y);
    void (APIENTRY * VertexAttrib2fv)(GLuint indx, const GLfloat* values);
    void (APIENTRY * VertexAttrib3f)(GLuint indx, GLfloat x, GLfloat y, GLfloat z);
    void (APIENTRY * VertexAttrib3fv)(GLuint indx, const GLfloat* values);
    void (APIENTRY * VertexAttrib4f)(GLuint indx, GLfloat x, GLfloat y, GLfloat z, GLfloat w);
    void (APIENTRY * VertexAttrib4fv)(GLuint indx, const GLfloat* values);
    void (APIENTRY * VertexAttribPointer)(GLuint indx, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid* ptr);

protected:
    HMODULE m_lib;
    LibType m_libraryType;
    bool m_loaded;
};

class QWindowsOpenGL : public QAbstractWindowsOpenGL
{
public:
    QWindowsOpenGL();
    ~QWindowsOpenGL();

private:
    bool load(const char *glName, const char *eglName);
    void unload();

    void resolve();

    void resolveWGL();
    void resolveEGL();
    void resolveGLCommon();
    void resolveGL11();
    void resolveGLES2();

    FARPROC resolveFunc(const char *name);
    FARPROC resolveEglFunc(const char *name);

    bool testDesktopGL();

    HMODULE m_eglLib;
};

static QString qgl_windowsErrorMessage(unsigned long errorCode)
{
    QString rc = QString::fromLatin1("#%1: ").arg(errorCode);
    ushort *lpMsgBuf;

    const int len = FormatMessage(
            FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL, errorCode, 0, (LPTSTR)&lpMsgBuf, 0, NULL);
    if (len) {
        rc += QString::fromUtf16(lpMsgBuf, len);
        LocalFree(lpMsgBuf);
    } else {
        rc += QString::fromLatin1("<unknown error>");
    }
    return rc;
}

static HMODULE qgl_loadLib(const char *name, bool warnOnFail = true)
{
    HMODULE lib = LoadLibraryA(name);

    if (lib)
        return lib;

    if (warnOnFail) {
        QString msg = qgl_windowsErrorMessage(GetLastError());
        qCWarning(qglLc, "Failed to load %s: %s", name, qPrintable(msg));
    }

    return 0;
}

QWindowsOpenGL::QWindowsOpenGL()
    : m_eglLib(0)
{
    if (qEnvironmentVariableIsSet("QT_OPENGLPROXY_DEBUG"))
        QLoggingCategory::setFilterRules(QStringLiteral("qt.gui.openglproxy=true"));

    enum RequestedLib {
        Unknown,
        Desktop,
        GLES
    } req = Unknown;

    // Check if the application has requested a certain implementation.
    if (QCoreApplication::testAttribute(Qt::AA_UseDesktopOpenGL))
        req = Desktop;
    else if (QCoreApplication::testAttribute(Qt::AA_UseOpenGLES))
        req = GLES;

    // Check if an implementation is forced through the environment variable.
    QByteArray requested = qgetenv("QT_OPENGL");
    if (requested == QByteArrayLiteral("desktop"))
        req = Desktop;
    else if (requested == QByteArrayLiteral("angle"))
        req = GLES;

    bool desktopTested = false;
    if (req == Unknown) {
        // No explicit request. Start testing. opengl32.dll is preferred. Angle is the fallback.
        desktopTested = true;
        if (testDesktopGL())
            req = Desktop;
        else
            req = GLES;
    }

    Q_ASSERT(req != Unknown);

    if (req == GLES) {
        qCDebug(qglLc, "Using Angle");
#ifdef QT_DEBUG
        m_loaded = load("libglesv2d.dll", "libegld.dll");
#else
        m_loaded = load("libglesv2.dll", "libegl.dll");
#endif
        if (m_loaded) {
            m_libraryType = QWindowsOpenGL::GLES2;
        } else {
            // Could not load Angle. Try opengl32.dll.
            if (!desktopTested && testDesktopGL())
                req = Desktop;
        }
    }

    if (req == Desktop) {
        qCDebug(qglLc, "Using desktop OpenGL");
        m_loaded = load("opengl32.dll", 0);
        if (m_loaded)
            m_libraryType = QWindowsOpenGL::DesktopGL;
    }

    if (m_loaded)
        resolve();

    // When no library is loaded, keep on running. All EGL/WGL/GL functions will
    // return 0 in this case without further errors. It is up to the clients
    // (application code, Qt Quick, etc.) to act when eglInitialize() and
    // friends fail, i.e. when QOpenGLContext::create() returns false due to the
    // platform plugin's failure to create a platform context.
}

QWindowsOpenGL::~QWindowsOpenGL()
{
    unload();
}

bool QWindowsOpenGL::load(const char *glName, const char *eglName)
{
    qCDebug(qglLc, "Loading %s %s", glName, eglName ? eglName : "");

    bool result = true;

    if (glName) {
        m_lib = qgl_loadLib(glName);
        result &= m_lib != 0;
    }

    if (eglName) {
        m_eglLib = qgl_loadLib(eglName);
        result &= m_eglLib != 0;
    }

    if (!result)
        unload();

    return result;
}

void QWindowsOpenGL::unload()
{
    if (m_lib) {
        FreeLibrary(m_lib);
        m_lib = 0;
    }
    if (m_eglLib) {
        FreeLibrary(m_eglLib);
        m_eglLib = 0;
    }
    m_loaded = false;
}

FARPROC QWindowsOpenGL::resolveFunc(const char *name)
{
    FARPROC proc = m_lib ? ::GetProcAddress(m_lib, name) : 0;
    if (!proc)
        qCDebug(qglLc, "Failed to resolve GL function %s", name);
    return proc;
}

FARPROC QWindowsOpenGL::resolveEglFunc(const char *name)
{
    FARPROC proc = m_eglLib ? ::GetProcAddress(m_eglLib, name) : 0;
    if (!proc)
        qCDebug(qglLc, "Failed to resolve EGL function %s", name);
    return proc;
}

void QWindowsOpenGL::resolveWGL()
{
    CopyContext = reinterpret_cast<BOOL (WINAPI *)(HGLRC, HGLRC, UINT)>(resolveFunc("wglCopyContext"));
    CreateContext = reinterpret_cast<HGLRC (WINAPI *)(HDC)>(resolveFunc("wglCreateContext"));
    CreateLayerContext = reinterpret_cast<HGLRC (WINAPI *)(HDC, int)>(resolveFunc("wglCreateLayerContext"));
    DeleteContext = reinterpret_cast<BOOL (WINAPI *)(HGLRC)>(resolveFunc("wglDeleteContext"));
    GetCurrentContext = reinterpret_cast<HGLRC (WINAPI *)()>(resolveFunc("wglGetCurrentContext"));
    GetCurrentDC = reinterpret_cast<HDC (WINAPI *)()>(resolveFunc("wglGetCurrentDC"));
    GetProcAddress = reinterpret_cast<PROC (WINAPI *)(LPCSTR)>(resolveFunc("wglGetProcAddress"));
    MakeCurrent = reinterpret_cast<BOOL (WINAPI *)(HDC, HGLRC)>(resolveFunc("wglMakeCurrent"));
    ShareLists = reinterpret_cast<BOOL (WINAPI *)(HGLRC, HGLRC)>(resolveFunc("wglShareLists"));
    UseFontBitmapsW = reinterpret_cast<BOOL (WINAPI *)(HDC, DWORD, DWORD, DWORD)>(resolveFunc("wglUseFontBitmapsW"));
    UseFontOutlinesW = reinterpret_cast<BOOL (WINAPI *)(HDC, DWORD, DWORD, DWORD, FLOAT, FLOAT, int, LPGLYPHMETRICSFLOAT)>(resolveFunc("wglUseFontOutlinesW"));
    DescribeLayerPlane = reinterpret_cast<BOOL (WINAPI *)(HDC, int, int, UINT, LPLAYERPLANEDESCRIPTOR)>(resolveFunc("wglDescribeLayerPlane"));
    SetLayerPaletteEntries = reinterpret_cast<int (WINAPI *)(HDC, int, int, int, CONST COLORREF *)>(resolveFunc("wglSetLayerPaletteEntries"));
    GetLayerPaletteEntries = reinterpret_cast<int (WINAPI *)(HDC, int, int, int, COLORREF *)>(resolveFunc("wglGetLayerPaletteEntries"));
    RealizeLayerPalette = reinterpret_cast<BOOL (WINAPI *)(HDC, int, BOOL)>(resolveFunc("wglRealizeLayerPalette"));
    SwapLayerBuffers = reinterpret_cast<BOOL (WINAPI *)(HDC, UINT)>(resolveFunc("wglSwapLayerBuffers"));
    SwapMultipleBuffers = reinterpret_cast<DWORD (WINAPI *)(UINT, CONST WGLSWAP *)>(resolveFunc("wglSwapMultipleBuffers"));
}

void QWindowsOpenGL::resolveEGL()
{
    EGL_GetError = reinterpret_cast<EGLint (EGLAPIENTRY *)(void)>(resolveEglFunc("eglGetError"));
    EGL_GetDisplay = reinterpret_cast<EGLDisplay (EGLAPIENTRY *)(EGLNativeDisplayType)>(resolveEglFunc("eglGetDisplay"));
    EGL_Initialize = reinterpret_cast<EGLBoolean (EGLAPIENTRY *)(EGLDisplay, EGLint *, EGLint *)>(resolveEglFunc("eglInitialize"));
    EGL_Terminate = reinterpret_cast<EGLBoolean (EGLAPIENTRY *)(EGLDisplay)>(resolveEglFunc("eglTerminate"));
    EGL_QueryString = reinterpret_cast<const char * (EGLAPIENTRY *)(EGLDisplay, EGLint)>(resolveEglFunc("eglQueryString"));
    EGL_GetConfigs = reinterpret_cast<EGLBoolean (EGLAPIENTRY * )(EGLDisplay, EGLConfig *, EGLint, EGLint *)>(resolveEglFunc("eglGetConfigs"));
    EGL_ChooseConfig = reinterpret_cast<EGLBoolean (EGLAPIENTRY *)(EGLDisplay, const EGLint *, EGLConfig *, EGLint, EGLint *)>(resolveEglFunc("eglChooseConfig"));
    EGL_GetConfigAttrib = reinterpret_cast<EGLBoolean (EGLAPIENTRY *)(EGLDisplay, EGLConfig, EGLint, EGLint *)>(resolveEglFunc("eglGetConfigAttrib"));
    EGL_CreateWindowSurface = reinterpret_cast<EGLSurface (EGLAPIENTRY *)(EGLDisplay, EGLConfig, EGLNativeWindowType, const EGLint *)>(resolveEglFunc("eglCreateWindowSurface"));
    EGL_CreatePbufferSurface = reinterpret_cast<EGLSurface (EGLAPIENTRY *)(EGLDisplay , EGLConfig, const EGLint *)>(resolveEglFunc("eglCreatePbufferSurface"));
    EGL_CreatePixmapSurface = reinterpret_cast<EGLSurface (EGLAPIENTRY * )(EGLDisplay dpy, EGLConfig config, EGLNativePixmapType , const EGLint *)>(resolveEglFunc("eglCreatePixmapSurface"));
    EGL_DestroySurface = reinterpret_cast<EGLBoolean (EGLAPIENTRY *)(EGLDisplay , EGLSurface )>(resolveEglFunc("eglDestroySurface"));
    EGL_QuerySurface = reinterpret_cast<EGLBoolean (EGLAPIENTRY * )(EGLDisplay , EGLSurface , EGLint , EGLint *)>(resolveEglFunc("eglQuerySurface"));
    EGL_BindAPI = reinterpret_cast<EGLBoolean (EGLAPIENTRY * )(EGLenum )>(resolveEglFunc("eglBindAPI"));
    EGL_QueryAPI = reinterpret_cast<EGLenum (EGLAPIENTRY *)(void)>(resolveEglFunc("eglQueryAPI"));
    EGL_WaitClient = reinterpret_cast<EGLBoolean (EGLAPIENTRY *)(void)>(resolveEglFunc("eglWaitClient"));
    EGL_ReleaseThread = reinterpret_cast<EGLBoolean (EGLAPIENTRY *)(void)>(resolveEglFunc("eglReleaseThread"));
    EGL_CreatePbufferFromClientBuffer = reinterpret_cast<EGLSurface (EGLAPIENTRY * )(EGLDisplay , EGLenum , EGLClientBuffer , EGLConfig , const EGLint *)>(resolveEglFunc("eglCreatePbufferFromClientBuffer"));
    EGL_SurfaceAttrib = reinterpret_cast<EGLBoolean (EGLAPIENTRY *)(EGLDisplay , EGLSurface , EGLint , EGLint )>(resolveEglFunc("eglSurfaceAttrib"));
    EGL_BindTexImage = reinterpret_cast<EGLBoolean (EGLAPIENTRY * )(EGLDisplay, EGLSurface , EGLint )>(resolveEglFunc("eglBindTexImage"));
    EGL_ReleaseTexImage = reinterpret_cast<EGLBoolean (EGLAPIENTRY *)(EGLDisplay, EGLSurface , EGLint)>(resolveEglFunc("eglReleaseTexImage"));
    EGL_SwapInterval = reinterpret_cast<EGLBoolean (EGLAPIENTRY *)(EGLDisplay , EGLint )>(resolveEglFunc("eglSwapInterval"));
    EGL_CreateContext = reinterpret_cast<EGLContext (EGLAPIENTRY *)(EGLDisplay , EGLConfig , EGLContext , const EGLint *)>(resolveEglFunc("eglCreateContext"));
    EGL_DestroyContext = reinterpret_cast<EGLBoolean (EGLAPIENTRY *)(EGLDisplay, EGLContext)>(resolveEglFunc("eglDestroyContext"));
    EGL_MakeCurrent  = reinterpret_cast<EGLBoolean (EGLAPIENTRY *)(EGLDisplay , EGLSurface , EGLSurface , EGLContext )>(resolveEglFunc("eglMakeCurrent"));
    EGL_GetCurrentContext = reinterpret_cast<EGLContext (EGLAPIENTRY *)(void)>(resolveEglFunc("eglGetCurrentContext"));
    EGL_GetCurrentSurface = reinterpret_cast<EGLSurface (EGLAPIENTRY *)(EGLint )>(resolveEglFunc("eglGetCurrentSurface"));
    EGL_GetCurrentDisplay = reinterpret_cast<EGLDisplay (EGLAPIENTRY *)(void)>(resolveEglFunc("eglGetCurrentDisplay"));
    EGL_QueryContext = reinterpret_cast<EGLBoolean (EGLAPIENTRY *)(EGLDisplay , EGLContext , EGLint , EGLint *)>(resolveEglFunc("eglQueryContext"));
    EGL_WaitGL = reinterpret_cast<EGLBoolean (EGLAPIENTRY *)(void)>(resolveEglFunc("eglWaitGL"));
    EGL_WaitNative = reinterpret_cast<EGLBoolean (EGLAPIENTRY *)(EGLint )>(resolveEglFunc("eglWaitNative"));
    EGL_SwapBuffers = reinterpret_cast<EGLBoolean (EGLAPIENTRY *)(EGLDisplay , EGLSurface)>(resolveEglFunc("eglSwapBuffers"));
    EGL_CopyBuffers = reinterpret_cast<EGLBoolean (EGLAPIENTRY *)(EGLDisplay , EGLSurface , EGLNativePixmapType )>(resolveEglFunc("eglCopyBuffers"));
    EGL_GetProcAddress = reinterpret_cast<__eglMustCastToProperFunctionPointerType (EGLAPIENTRY * )(const char *)>(resolveEglFunc("eglGetProcAddress"));
}

void QWindowsOpenGL::resolveGLCommon()
{
    Viewport = reinterpret_cast<void (APIENTRY *)(GLint , GLint , GLsizei , GLsizei )>(resolveFunc("glViewport"));
    IsEnabled = reinterpret_cast<GLboolean (APIENTRY *)(GLenum )>(resolveFunc("glIsEnabled"));
    GetTexParameteriv = reinterpret_cast<void (APIENTRY *)(GLenum , GLenum , GLint *)>(resolveFunc("glGetTexParameteriv"));
    GetTexParameterfv = reinterpret_cast<void (APIENTRY *)(GLenum , GLenum , GLfloat *)>(resolveFunc("glGetTexParameterfv"));
    GetString = reinterpret_cast<const GLubyte * (APIENTRY *)(GLenum )>(resolveFunc("glGetString"));
    GetIntegerv = reinterpret_cast<void (APIENTRY *)(GLenum , GLint *)>(resolveFunc("glGetIntegerv"));
    GetFloatv = reinterpret_cast<void (APIENTRY *)(GLenum , GLfloat *)>(resolveFunc("glGetFloatv"));
    GetError = reinterpret_cast<GLenum (APIENTRY *)()>(resolveFunc("glGetError"));
    GetBooleanv = reinterpret_cast<void (APIENTRY *)(GLenum , GLboolean *)>(resolveFunc("glGetBooleanv"));
    ReadPixels = reinterpret_cast<void (APIENTRY *)(GLint , GLint , GLsizei , GLsizei , GLenum , GLenum , GLvoid *)>(resolveFunc("glReadPixels"));
    PixelStorei = reinterpret_cast<void (APIENTRY *)(GLenum , GLint )>(resolveFunc("glPixelStorei"));
    DepthFunc = reinterpret_cast<void (APIENTRY *)(GLenum )>(resolveFunc("glDepthFunc"));
    StencilOp = reinterpret_cast<void (APIENTRY *)(GLenum , GLenum , GLenum )>(resolveFunc("glStencilOp"));
    StencilFunc = reinterpret_cast<void (APIENTRY *)(GLenum , GLint , GLuint )>(resolveFunc("glStencilFunc"));
    BlendFunc = reinterpret_cast<void (APIENTRY *)(GLenum , GLenum )>(resolveFunc("glBlendFunc"));
    Flush = reinterpret_cast<void (APIENTRY *)()>(resolveFunc("glFlush"));
    Finish = reinterpret_cast<void (APIENTRY *)()>(resolveFunc("glFinish"));
    Enable = reinterpret_cast<void (APIENTRY *)(GLenum )>(resolveFunc("glEnable"));
    Disable = reinterpret_cast<void (APIENTRY *)(GLenum )>(resolveFunc("glDisable"));
    DepthMask = reinterpret_cast<void (APIENTRY *)(GLboolean )>(resolveFunc("glDepthMask"));
    ColorMask = reinterpret_cast<void (APIENTRY *)(GLboolean , GLboolean , GLboolean , GLboolean )>(resolveFunc("glColorMask"));
    StencilMask = reinterpret_cast<void (APIENTRY *)(GLuint )>(resolveFunc("glStencilMask"));
    ClearStencil = reinterpret_cast<void (APIENTRY *)(GLint )>(resolveFunc("glClearStencil"));
    ClearColor = reinterpret_cast<void (APIENTRY *)(GLfloat , GLfloat , GLfloat , GLfloat )>(resolveFunc("glClearColor"));
    Clear = reinterpret_cast<void (APIENTRY *)(GLbitfield )>(resolveFunc("glClear"));
    TexImage2D = reinterpret_cast<void (APIENTRY *)(GLenum , GLint , GLint , GLsizei , GLsizei , GLint , GLenum , GLenum , const GLvoid *)>(resolveFunc("glTexImage2D"));
    TexParameteriv = reinterpret_cast<void (APIENTRY *)(GLenum , GLenum , const GLint *)>(resolveFunc("glTexParameteriv"));
    TexParameteri = reinterpret_cast<void (APIENTRY *)(GLenum , GLenum , GLint )>(resolveFunc("glTexParameteri"));
    TexParameterfv = reinterpret_cast<void (APIENTRY *)(GLenum , GLenum , const GLfloat *)>(resolveFunc("glTexParameterfv"));
    TexParameterf = reinterpret_cast<void (APIENTRY *)(GLenum , GLenum , GLfloat )>(resolveFunc("glTexParameterf"));
    Scissor = reinterpret_cast<void (APIENTRY *)(GLint , GLint , GLsizei , GLsizei )>(resolveFunc("glScissor"));
    LineWidth = reinterpret_cast<void (APIENTRY *)(GLfloat )>(resolveFunc("glLineWidth"));
    Hint = reinterpret_cast<void (APIENTRY *)(GLenum , GLenum )>(resolveFunc("glHint"));
    FrontFace = reinterpret_cast<void (APIENTRY *)(GLenum )>(resolveFunc("glFrontFace"));
    CullFace = reinterpret_cast<void (APIENTRY *)(GLenum )>(resolveFunc("glCullFace"));

    IsTexture = reinterpret_cast<GLboolean (APIENTRY *)(GLuint )>(resolveFunc("glIsTexture"));
    GenTextures = reinterpret_cast<void (APIENTRY *)(GLsizei , GLuint *)>(resolveFunc("glGenTextures"));
    DeleteTextures = reinterpret_cast<void (APIENTRY *)(GLsizei , const GLuint *)>(resolveFunc("glDeleteTextures"));
    BindTexture = reinterpret_cast<void (APIENTRY *)(GLenum , GLuint )>(resolveFunc("glBindTexture"));
    TexSubImage2D = reinterpret_cast<void (APIENTRY *)(GLenum , GLint , GLint , GLint , GLsizei , GLsizei , GLenum , GLenum , const GLvoid *)>(resolveFunc("glTexSubImage2D"));
    CopyTexSubImage2D = reinterpret_cast<void (APIENTRY *)(GLenum , GLint , GLint , GLint , GLint , GLint , GLsizei , GLsizei )>(resolveFunc("glCopyTexSubImage2D"));
    CopyTexImage2D = reinterpret_cast<void (APIENTRY *)(GLenum , GLint , GLenum , GLint , GLint , GLsizei , GLsizei , GLint )>(resolveFunc("glCopyTexImage2D"));
    PolygonOffset = reinterpret_cast<void (APIENTRY *)(GLfloat , GLfloat )>(resolveFunc("glPolygonOffset"));
    DrawElements = reinterpret_cast<void (APIENTRY *)(GLenum , GLsizei , GLenum , const GLvoid *)>(resolveFunc("glDrawElements"));
    DrawArrays = reinterpret_cast<void (APIENTRY *)(GLenum , GLint , GLsizei )>(resolveFunc("glDrawArrays"));
}

void QWindowsOpenGL::resolveGL11()
{
    DepthRange = reinterpret_cast<void (APIENTRY *)(GLdouble , GLdouble )>(resolveFunc("glDepthRange"));
    GetTexImage = reinterpret_cast<void (APIENTRY *)(GLenum , GLint , GLenum , GLenum , GLvoid *)>(resolveFunc("glGetTexImage"));
    LogicOp = reinterpret_cast<void (APIENTRY *)(GLenum )>(resolveFunc("glLogicOp"));
    ClearDepth = reinterpret_cast<void (APIENTRY *)(GLdouble )>(resolveFunc("glClearDepth"));
    PolygonMode = reinterpret_cast<void (APIENTRY *)(GLenum , GLenum )>(resolveFunc("glPolygonMode"));
    PointSize = reinterpret_cast<void (APIENTRY *)(GLfloat )>(resolveFunc("glPointSize"));
    GetTexLevelParameteriv = reinterpret_cast<void (APIENTRY *)(GLenum , GLint , GLenum , GLint *)>(resolveFunc("glGetTexLevelParameteriv"));
    GetTexLevelParameterfv = reinterpret_cast<void (APIENTRY *)(GLenum , GLint , GLenum , GLfloat *)>(resolveFunc("glGetTexLevelParameterfv"));
    GetDoublev = reinterpret_cast<void (APIENTRY *)(GLenum , GLdouble *)>(resolveFunc("glGetDoublev"));
    PixelStoref = reinterpret_cast<void (APIENTRY *)(GLenum , GLfloat )>(resolveFunc("glPixelStoref"));
    ReadBuffer = reinterpret_cast<void (APIENTRY *)(GLenum )>(resolveFunc("glReadBuffer"));
    DrawBuffer = reinterpret_cast<void (APIENTRY *)(GLenum )>(resolveFunc("glDrawBuffer"));
    TexImage1D = reinterpret_cast<void (APIENTRY *)(GLenum , GLint , GLint , GLsizei , GLint , GLenum , GLenum , const GLvoid *)>(resolveFunc("glTexImage1D"));

    Translatef = reinterpret_cast<void (APIENTRY *)(GLfloat , GLfloat , GLfloat )>(resolveFunc("glTranslatef"));
    Translated = reinterpret_cast<void (APIENTRY *)(GLdouble , GLdouble , GLdouble )>(resolveFunc("glTranslated"));
    Scalef = reinterpret_cast<void (APIENTRY *)(GLfloat , GLfloat , GLfloat )>(resolveFunc("glScalef"));
    Scaled = reinterpret_cast<void (APIENTRY *)(GLdouble , GLdouble , GLdouble )>(resolveFunc("glScaled"));
    Rotatef = reinterpret_cast<void (APIENTRY *)(GLfloat , GLfloat , GLfloat , GLfloat )>(resolveFunc("glRotatef"));
    Rotated = reinterpret_cast<void (APIENTRY *)(GLdouble , GLdouble , GLdouble , GLdouble )>(resolveFunc("glRotated"));
    PushMatrix = reinterpret_cast<void (APIENTRY *)()>(resolveFunc("glPushMatrix"));
    PopMatrix = reinterpret_cast<void (APIENTRY *)()>(resolveFunc("glPopMatrix"));
    Ortho = reinterpret_cast<void (APIENTRY *)(GLdouble , GLdouble , GLdouble , GLdouble , GLdouble , GLdouble )>(resolveFunc("glOrtho"));
    MultMatrixd = reinterpret_cast<void (APIENTRY *)(const GLdouble *)>(resolveFunc("glMultMatrixd"));
    MultMatrixf = reinterpret_cast<void (APIENTRY *)(const GLfloat *)>(resolveFunc("glMultMatrixf"));
    MatrixMode = reinterpret_cast<void (APIENTRY *)(GLenum )>(resolveFunc("glMatrixMode"));
    LoadMatrixd = reinterpret_cast<void (APIENTRY *)(const GLdouble *)>(resolveFunc("glLoadMatrixd"));
    LoadMatrixf = reinterpret_cast<void (APIENTRY *)(const GLfloat *)>(resolveFunc("glLoadMatrixf"));
    LoadIdentity = reinterpret_cast<void (APIENTRY *)()>(resolveFunc("glLoadIdentity"));
    Frustum = reinterpret_cast<void (APIENTRY *)(GLdouble , GLdouble , GLdouble , GLdouble , GLdouble , GLdouble )>(resolveFunc("glFrustum"));
    IsList = reinterpret_cast<GLboolean (APIENTRY *)(GLuint )>(resolveFunc("glIsList"));
    GetTexGeniv = reinterpret_cast<void (APIENTRY *)(GLenum , GLenum , GLint *)>(resolveFunc("glGetTexGeniv"));
    GetTexGenfv = reinterpret_cast<void (APIENTRY *)(GLenum , GLenum , GLfloat *)>(resolveFunc("glGetTexGenfv"));
    GetTexGendv = reinterpret_cast<void (APIENTRY *)(GLenum , GLenum , GLdouble *)>(resolveFunc("glGetTexGendv"));
    GetTexEnviv = reinterpret_cast<void (APIENTRY *)(GLenum , GLenum , GLint *)>(resolveFunc("glGetTexEnviv"));
    GetTexEnvfv = reinterpret_cast<void (APIENTRY *)(GLenum , GLenum , GLfloat *)>(resolveFunc("glGetTexEnvfv"));
    GetPolygonStipple = reinterpret_cast<void (APIENTRY *)(GLubyte *)>(resolveFunc("glGetPolygonStipple"));
    GetPixelMapusv = reinterpret_cast<void (APIENTRY *)(GLenum , GLushort *)>(resolveFunc("glGetPixelMapusv"));
    GetPixelMapuiv = reinterpret_cast<void (APIENTRY *)(GLenum , GLuint *)>(resolveFunc("glGetPixelMapuiv"));
    GetPixelMapfv = reinterpret_cast<void (APIENTRY *)(GLenum , GLfloat *)>(resolveFunc("glGetPixelMapfv"));
    GetMaterialiv = reinterpret_cast<void (APIENTRY *)(GLenum , GLenum , GLint *)>(resolveFunc("glGetMaterialiv"));
    GetMaterialfv = reinterpret_cast<void (APIENTRY *)(GLenum , GLenum , GLfloat *)>(resolveFunc("glGetMaterialfv"));
    GetMapiv = reinterpret_cast<void (APIENTRY *)(GLenum , GLenum , GLint *)>(resolveFunc("glGetMapiv"));
    GetMapfv = reinterpret_cast<void (APIENTRY *)(GLenum , GLenum , GLfloat *)>(resolveFunc("glGetMapfv"));
    GetMapdv = reinterpret_cast<void (APIENTRY *)(GLenum , GLenum , GLdouble *)>(resolveFunc("glGetMapdv"));
    GetLightiv = reinterpret_cast<void (APIENTRY *)(GLenum , GLenum , GLint *)>(resolveFunc("glGetLightiv"));
    GetLightfv = reinterpret_cast<void (APIENTRY *)(GLenum , GLenum , GLfloat *)>(resolveFunc("glGetLightfv"));
    GetClipPlane = reinterpret_cast<void (APIENTRY *)(GLenum , GLdouble *)>(resolveFunc("glGetClipPlane"));
    DrawPixels = reinterpret_cast<void (APIENTRY *)(GLsizei , GLsizei , GLenum , GLenum , const GLvoid *)>(resolveFunc("glDrawPixels"));
    CopyPixels = reinterpret_cast<void (APIENTRY *)(GLint , GLint , GLsizei , GLsizei , GLenum )>(resolveFunc("glCopyPixels"));
    PixelMapusv = reinterpret_cast<void (APIENTRY *)(GLenum , GLint , const GLushort *)>(resolveFunc("glPixelMapusv"));
    PixelMapuiv = reinterpret_cast<void (APIENTRY *)(GLenum , GLint , const GLuint *)>(resolveFunc("glPixelMapuiv"));
    PixelMapfv = reinterpret_cast<void (APIENTRY *)(GLenum , GLint , const GLfloat *)>(resolveFunc("glPixelMapfv"));
    PixelTransferi = reinterpret_cast<void (APIENTRY *)(GLenum , GLint )>(resolveFunc("glPixelTransferi"));
    PixelTransferf = reinterpret_cast<void (APIENTRY *)(GLenum , GLfloat )>(resolveFunc("glPixelTransferf"));
    PixelZoom = reinterpret_cast<void (APIENTRY *)(GLfloat , GLfloat )>(resolveFunc("glPixelZoom"));
    AlphaFunc = reinterpret_cast<void (APIENTRY *)(GLenum , GLfloat )>(resolveFunc("glAlphaFunc"));
    EvalPoint2 = reinterpret_cast<void (APIENTRY *)(GLint , GLint )>(resolveFunc("glEvalPoint2"));
    EvalMesh2 = reinterpret_cast<void (APIENTRY *)(GLenum , GLint , GLint , GLint , GLint )>(resolveFunc("glEvalMesh2"));
    EvalPoint1 = reinterpret_cast<void (APIENTRY *)(GLint )>(resolveFunc("glEvalPoint1"));
    EvalMesh1 = reinterpret_cast<void (APIENTRY *)(GLenum , GLint , GLint )>(resolveFunc("glEvalMesh1"));
    EvalCoord2fv = reinterpret_cast<void (APIENTRY *)(const GLfloat *)>(resolveFunc("glEvalCoord2fv"));
    EvalCoord2f = reinterpret_cast<void (APIENTRY *)(GLfloat , GLfloat )>(resolveFunc("glEvalCoord2f"));
    EvalCoord2dv = reinterpret_cast<void (APIENTRY *)(const GLdouble *)>(resolveFunc("glEvalCoord2dv"));
    EvalCoord2d = reinterpret_cast<void (APIENTRY *)(GLdouble , GLdouble )>(resolveFunc("glEvalCoord2d"));
    EvalCoord1fv = reinterpret_cast<void (APIENTRY *)(const GLfloat *)>(resolveFunc("glEvalCoord1fv"));
    EvalCoord1f = reinterpret_cast<void (APIENTRY *)(GLfloat )>(resolveFunc("glEvalCoord1f"));
    EvalCoord1dv = reinterpret_cast<void (APIENTRY *)(const GLdouble *)>(resolveFunc("glEvalCoord1dv"));
    EvalCoord1d = reinterpret_cast<void (APIENTRY *)(GLdouble )>(resolveFunc("glEvalCoord1d"));
    MapGrid2f = reinterpret_cast<void (APIENTRY *)(GLint , GLfloat , GLfloat , GLint , GLfloat , GLfloat )>(resolveFunc("glMapGrid2f"));
    MapGrid2d = reinterpret_cast<void (APIENTRY *)(GLint , GLdouble , GLdouble , GLint , GLdouble , GLdouble )>(resolveFunc("glMapGrid2d"));
    MapGrid1f = reinterpret_cast<void (APIENTRY *)(GLint , GLfloat , GLfloat )>(resolveFunc("glMapGrid1f"));
    MapGrid1d = reinterpret_cast<void (APIENTRY *)(GLint , GLdouble , GLdouble )>(resolveFunc("glMapGrid1d"));
    Map2f = reinterpret_cast<void (APIENTRY *)(GLenum , GLfloat , GLfloat , GLint , GLint , GLfloat , GLfloat , GLint , GLint , const GLfloat *)>(resolveFunc("glMap2f"));
    Map2d = reinterpret_cast<void (APIENTRY *)(GLenum , GLdouble , GLdouble , GLint , GLint , GLdouble , GLdouble , GLint , GLint , const GLdouble *)>(resolveFunc("glMap2d"));
    Map1f = reinterpret_cast<void (APIENTRY *)(GLenum , GLfloat , GLfloat , GLint , GLint , const GLfloat *)>(resolveFunc("glMap1f"));
    Map1d = reinterpret_cast<void (APIENTRY *)(GLenum , GLdouble , GLdouble , GLint , GLint , const GLdouble *)>(resolveFunc("glMap1d"));
    PushAttrib = reinterpret_cast<void (APIENTRY *)(GLbitfield )>(resolveFunc("glPushAttrib"));
    PopAttrib = reinterpret_cast<void (APIENTRY *)()>(resolveFunc("glPopAttrib"));
    Accum = reinterpret_cast<void (APIENTRY *)(GLenum , GLfloat )>(resolveFunc("glAccum"));
    IndexMask = reinterpret_cast<void (APIENTRY *)(GLuint )>(resolveFunc("glIndexMask"));
    ClearIndex = reinterpret_cast<void (APIENTRY *)(GLfloat )>(resolveFunc("glClearIndex"));
    ClearAccum = reinterpret_cast<void (APIENTRY *)(GLfloat , GLfloat , GLfloat , GLfloat )>(resolveFunc("glClearAccum"));
    PushName = reinterpret_cast<void (APIENTRY *)(GLuint )>(resolveFunc("glPushName"));
    PopName = reinterpret_cast<void (APIENTRY *)()>(resolveFunc("glPopName"));
    PassThrough = reinterpret_cast<void (APIENTRY *)(GLfloat )>(resolveFunc("glPassThrough"));
    LoadName = reinterpret_cast<void (APIENTRY *)(GLuint )>(resolveFunc("glLoadName"));
    InitNames = reinterpret_cast<void (APIENTRY *)()>(resolveFunc("glInitNames"));
    RenderMode = reinterpret_cast<GLint (APIENTRY *)(GLenum )>(resolveFunc("glRenderMode"));
    SelectBuffer = reinterpret_cast<void (APIENTRY *)(GLsizei , GLuint *)>(resolveFunc("glSelectBuffer"));
    FeedbackBuffer = reinterpret_cast<void (APIENTRY *)(GLsizei , GLenum , GLfloat *)>(resolveFunc("glFeedbackBuffer"));
    TexGeniv = reinterpret_cast<void (APIENTRY *)(GLenum , GLenum , const GLint *)>(resolveFunc("glTexGeniv"));
    TexGeni = reinterpret_cast<void (APIENTRY *)(GLenum , GLenum , GLint )>(resolveFunc("glTexGeni"));
    TexGenfv = reinterpret_cast<void (APIENTRY *)(GLenum , GLenum , const GLfloat *)>(resolveFunc("glTexGenfv"));
    TexGenf = reinterpret_cast<void (APIENTRY *)(GLenum , GLenum , GLfloat )>(resolveFunc("glTexGenf"));
    TexGendv = reinterpret_cast<void (APIENTRY *)(GLenum , GLenum , const GLdouble *)>(resolveFunc("glTexGendv"));
    TexGend = reinterpret_cast<void (APIENTRY *)(GLenum , GLenum , GLdouble )>(resolveFunc("glTexGend"));
    TexEnviv = reinterpret_cast<void (APIENTRY *)(GLenum , GLenum , const GLint *)>(resolveFunc("glTexEnviv"));
    TexEnvi = reinterpret_cast<void (APIENTRY *)(GLenum , GLenum , GLint )>(resolveFunc("glTexEnvi"));
    TexEnvfv = reinterpret_cast<void (APIENTRY *)(GLenum , GLenum , const GLfloat *)>(resolveFunc("glTexEnvfv"));
    TexEnvf = reinterpret_cast<void (APIENTRY *)(GLenum , GLenum , GLfloat )>(resolveFunc("glTexEnvf"));
    ShadeModel = reinterpret_cast<void (APIENTRY *)(GLenum )>(resolveFunc("glShadeModel"));
    PolygonStipple = reinterpret_cast<void (APIENTRY *)(const GLubyte *)>(resolveFunc("glPolygonStipple"));
    Materialiv = reinterpret_cast<void (APIENTRY *)(GLenum , GLenum , const GLint *)>(resolveFunc("glMaterialiv"));
    Materiali = reinterpret_cast<void (APIENTRY *)(GLenum , GLenum , GLint )>(resolveFunc("glMateriali"));
    Materialfv = reinterpret_cast<void (APIENTRY *)(GLenum , GLenum , const GLfloat *)>(resolveFunc("glMaterialfv"));
    Materialf = reinterpret_cast<void (APIENTRY *)(GLenum , GLenum , GLfloat )>(resolveFunc("glMaterialf"));
    LineStipple = reinterpret_cast<void (APIENTRY *)(GLint , GLushort )>(resolveFunc("glLineStipple"));
    LightModeliv = reinterpret_cast<void (APIENTRY *)(GLenum , const GLint *)>(resolveFunc("glLightModeliv"));
    LightModeli = reinterpret_cast<void (APIENTRY *)(GLenum , GLint )>(resolveFunc("glLightModeli"));
    LightModelfv = reinterpret_cast<void (APIENTRY *)(GLenum , const GLfloat *)>(resolveFunc("glLightModelfv"));
    LightModelf = reinterpret_cast<void (APIENTRY *)(GLenum , GLfloat )>(resolveFunc("glLightModelf"));
    Lightiv = reinterpret_cast<void (APIENTRY *)(GLenum , GLenum , const GLint *)>(resolveFunc("glLightiv"));
    Lighti = reinterpret_cast<void (APIENTRY *)(GLenum , GLenum , GLint )>(resolveFunc("glLighti"));
    Lightfv = reinterpret_cast<void (APIENTRY *)(GLenum , GLenum , const GLfloat *)>(resolveFunc("glLightfv"));
    Lightf = reinterpret_cast<void (APIENTRY *)(GLenum , GLenum , GLfloat )>(resolveFunc("glLightf"));
    Fogiv = reinterpret_cast<void (APIENTRY *)(GLenum , const GLint *)>(resolveFunc("glFogiv"));
    Fogi = reinterpret_cast<void (APIENTRY *)(GLenum , GLint )>(resolveFunc("glFogi"));
    Fogfv = reinterpret_cast<void (APIENTRY *)(GLenum , const GLfloat *)>(resolveFunc("glFogfv"));
    Fogf = reinterpret_cast<void (APIENTRY *)(GLenum , GLfloat )>(resolveFunc("glFogf"));
    ColorMaterial = reinterpret_cast<void (APIENTRY *)(GLenum , GLenum )>(resolveFunc("glColorMaterial"));
    ClipPlane = reinterpret_cast<void (APIENTRY *)(GLenum , const GLdouble *)>(resolveFunc("glClipPlane"));
    Vertex4sv = reinterpret_cast<void (APIENTRY *)(const GLshort *)>(resolveFunc("glVertex4sv"));
    Vertex4s = reinterpret_cast<void (APIENTRY *)(GLshort , GLshort , GLshort , GLshort )>(resolveFunc("glVertex4s"));
    Vertex4iv = reinterpret_cast<void (APIENTRY *)(const GLint *)>(resolveFunc("glVertex4iv"));
    Vertex4i = reinterpret_cast<void (APIENTRY *)(GLint , GLint , GLint , GLint )>(resolveFunc("glVertex4i"));
    Vertex4fv = reinterpret_cast<void (APIENTRY *)(const GLfloat *)>(resolveFunc("glVertex4fv"));
    Vertex4f = reinterpret_cast<void (APIENTRY *)(GLfloat , GLfloat , GLfloat , GLfloat )>(resolveFunc("glVertex4f"));
    Vertex4dv = reinterpret_cast<void (APIENTRY *)(const GLdouble *)>(resolveFunc("glVertex4dv"));
    Vertex4d = reinterpret_cast<void (APIENTRY *)(GLdouble , GLdouble , GLdouble , GLdouble )>(resolveFunc("glVertex4d"));
    Vertex3sv = reinterpret_cast<void (APIENTRY *)(const GLshort *)>(resolveFunc("glVertex3sv"));
    Vertex3s = reinterpret_cast<void (APIENTRY *)(GLshort , GLshort , GLshort )>(resolveFunc("glVertex3s"));
    Vertex3iv = reinterpret_cast<void (APIENTRY *)(const GLint *)>(resolveFunc("glVertex3iv"));
    Vertex3i = reinterpret_cast<void (APIENTRY *)(GLint , GLint , GLint )>(resolveFunc("glVertex3i"));
    Vertex3fv = reinterpret_cast<void (APIENTRY *)(const GLfloat *)>(resolveFunc("glVertex3fv"));
    Vertex3f = reinterpret_cast<void (APIENTRY *)(GLfloat , GLfloat , GLfloat )>(resolveFunc("glVertex3f"));
    Vertex3dv = reinterpret_cast<void (APIENTRY *)(const GLdouble *)>(resolveFunc("glVertex3dv"));
    Vertex3d = reinterpret_cast<void (APIENTRY *)(GLdouble , GLdouble , GLdouble )>(resolveFunc("glVertex3d"));
    Vertex2sv = reinterpret_cast<void (APIENTRY *)(const GLshort *)>(resolveFunc("glVertex2sv"));
    Vertex2s = reinterpret_cast<void (APIENTRY *)(GLshort , GLshort )>(resolveFunc("glVertex2s"));
    Vertex2iv = reinterpret_cast<void (APIENTRY *)(const GLint *)>(resolveFunc("glVertex2iv"));
    Vertex2i = reinterpret_cast<void (APIENTRY *)(GLint , GLint )>(resolveFunc("glVertex2i"));
    Vertex2fv = reinterpret_cast<void (APIENTRY *)(const GLfloat *)>(resolveFunc("glVertex2fv"));
    Vertex2f = reinterpret_cast<void (APIENTRY *)(GLfloat , GLfloat )>(resolveFunc("glVertex2f"));
    Vertex2dv = reinterpret_cast<void (APIENTRY *)(const GLdouble *)>(resolveFunc("glVertex2dv"));
    Vertex2d = reinterpret_cast<void (APIENTRY *)(GLdouble , GLdouble )>(resolveFunc("glVertex2d"));
    TexCoord4sv = reinterpret_cast<void (APIENTRY *)(const GLshort *)>(resolveFunc("glTexCoord4sv"));
    TexCoord4s = reinterpret_cast<void (APIENTRY *)(GLshort , GLshort , GLshort , GLshort )>(resolveFunc("glTexCoord4s"));
    TexCoord4iv = reinterpret_cast<void (APIENTRY *)(const GLint *)>(resolveFunc("glTexCoord4iv"));
    TexCoord4i = reinterpret_cast<void (APIENTRY *)(GLint , GLint , GLint , GLint )>(resolveFunc("glTexCoord4i"));
    TexCoord4fv = reinterpret_cast<void (APIENTRY *)(const GLfloat *)>(resolveFunc("glTexCoord4fv"));
    TexCoord4f = reinterpret_cast<void (APIENTRY *)(GLfloat , GLfloat , GLfloat , GLfloat )>(resolveFunc("glTexCoord4f"));
    TexCoord4dv = reinterpret_cast<void (APIENTRY *)(const GLdouble *)>(resolveFunc("glTexCoord4dv"));
    TexCoord4d = reinterpret_cast<void (APIENTRY *)(GLdouble , GLdouble , GLdouble , GLdouble )>(resolveFunc("glTexCoord4d"));
    TexCoord3sv = reinterpret_cast<void (APIENTRY *)(const GLshort *)>(resolveFunc("glTexCoord3sv"));
    TexCoord3s = reinterpret_cast<void (APIENTRY *)(GLshort , GLshort , GLshort )>(resolveFunc("glTexCoord3s"));
    TexCoord3iv = reinterpret_cast<void (APIENTRY *)(const GLint *)>(resolveFunc("glTexCoord3iv"));
    TexCoord3i = reinterpret_cast<void (APIENTRY *)(GLint , GLint , GLint )>(resolveFunc("glTexCoord3i"));
    TexCoord3fv = reinterpret_cast<void (APIENTRY *)(const GLfloat *)>(resolveFunc("glTexCoord3fv"));
    TexCoord3f = reinterpret_cast<void (APIENTRY *)(GLfloat , GLfloat , GLfloat )>(resolveFunc("glTexCoord3f"));
    TexCoord3dv = reinterpret_cast<void (APIENTRY *)(const GLdouble *)>(resolveFunc("glTexCoord3dv"));
    TexCoord3d = reinterpret_cast<void (APIENTRY *)(GLdouble , GLdouble , GLdouble )>(resolveFunc("glTexCoord3d"));
    TexCoord2sv = reinterpret_cast<void (APIENTRY *)(const GLshort *)>(resolveFunc("glTexCoord2sv"));
    TexCoord2s = reinterpret_cast<void (APIENTRY *)(GLshort , GLshort )>(resolveFunc("glTexCoord2s"));
    TexCoord2iv = reinterpret_cast<void (APIENTRY *)(const GLint *)>(resolveFunc("glTexCoord2iv"));
    TexCoord2i = reinterpret_cast<void (APIENTRY *)(GLint , GLint )>(resolveFunc("glTexCoord2i"));
    TexCoord2fv = reinterpret_cast<void (APIENTRY *)(const GLfloat *)>(resolveFunc("glTexCoord2fv"));
    TexCoord2f = reinterpret_cast<void (APIENTRY *)(GLfloat , GLfloat )>(resolveFunc("glTexCoord2f"));
    TexCoord2dv = reinterpret_cast<void (APIENTRY *)(const GLdouble *)>(resolveFunc("glTexCoord2dv"));
    TexCoord2d = reinterpret_cast<void (APIENTRY *)(GLdouble , GLdouble )>(resolveFunc("glTexCoord2d"));
    TexCoord1sv = reinterpret_cast<void (APIENTRY *)(const GLshort *)>(resolveFunc("glTexCoord1sv"));
    TexCoord1s = reinterpret_cast<void (APIENTRY *)(GLshort )>(resolveFunc("glTexCoord1s"));
    TexCoord1iv = reinterpret_cast<void (APIENTRY *)(const GLint *)>(resolveFunc("glTexCoord1iv"));
    TexCoord1i = reinterpret_cast<void (APIENTRY *)(GLint )>(resolveFunc("glTexCoord1i"));
    TexCoord1fv = reinterpret_cast<void (APIENTRY *)(const GLfloat *)>(resolveFunc("glTexCoord1fv"));
    TexCoord1f = reinterpret_cast<void (APIENTRY *)(GLfloat )>(resolveFunc("glTexCoord1f"));
    TexCoord1dv = reinterpret_cast<void (APIENTRY *)(const GLdouble *)>(resolveFunc("glTexCoord1dv"));
    TexCoord1d = reinterpret_cast<void (APIENTRY *)(GLdouble )>(resolveFunc("glTexCoord1d"));
    Rectsv = reinterpret_cast<void (APIENTRY *)(const GLshort *, const GLshort *)>(resolveFunc("glRectsv"));
    Rects = reinterpret_cast<void (APIENTRY *)(GLshort , GLshort , GLshort , GLshort )>(resolveFunc("glRects"));
    Rectiv = reinterpret_cast<void (APIENTRY *)(const GLint *, const GLint *)>(resolveFunc("glRectiv"));
    Recti = reinterpret_cast<void (APIENTRY *)(GLint , GLint , GLint , GLint )>(resolveFunc("glRecti"));
    Rectfv = reinterpret_cast<void (APIENTRY *)(const GLfloat *, const GLfloat *)>(resolveFunc("glRectfv"));
    Rectf = reinterpret_cast<void (APIENTRY *)(GLfloat , GLfloat , GLfloat , GLfloat )>(resolveFunc("glRectf"));
    Rectdv = reinterpret_cast<void (APIENTRY *)(const GLdouble *, const GLdouble *)>(resolveFunc("glRectdv"));
    Rectd = reinterpret_cast<void (APIENTRY *)(GLdouble , GLdouble , GLdouble , GLdouble )>(resolveFunc("glRectd"));
    RasterPos4sv = reinterpret_cast<void (APIENTRY *)(const GLshort *)>(resolveFunc("glRasterPos4sv"));
    RasterPos4s = reinterpret_cast<void (APIENTRY *)(GLshort , GLshort , GLshort , GLshort )>(resolveFunc("glRasterPos4s"));
    RasterPos4iv = reinterpret_cast<void (APIENTRY *)(const GLint *)>(resolveFunc("glRasterPos4iv"));
    RasterPos4i = reinterpret_cast<void (APIENTRY *)(GLint , GLint , GLint , GLint )>(resolveFunc("glRasterPos4i"));
    RasterPos4fv = reinterpret_cast<void (APIENTRY *)(const GLfloat *)>(resolveFunc("glRasterPos4fv"));
    RasterPos4f = reinterpret_cast<void (APIENTRY *)(GLfloat , GLfloat , GLfloat , GLfloat )>(resolveFunc("glRasterPos4f"));
    RasterPos4dv = reinterpret_cast<void (APIENTRY *)(const GLdouble *)>(resolveFunc("glRasterPos4dv"));
    RasterPos4d = reinterpret_cast<void (APIENTRY *)(GLdouble , GLdouble , GLdouble , GLdouble )>(resolveFunc("glRasterPos4d"));
    RasterPos3sv = reinterpret_cast<void (APIENTRY *)(const GLshort *)>(resolveFunc("glRasterPos3sv"));
    RasterPos3s = reinterpret_cast<void (APIENTRY *)(GLshort , GLshort , GLshort )>(resolveFunc("glRasterPos3s"));
    RasterPos3iv = reinterpret_cast<void (APIENTRY *)(const GLint *)>(resolveFunc("glRasterPos3iv"));
    RasterPos3i = reinterpret_cast<void (APIENTRY *)(GLint , GLint , GLint )>(resolveFunc("glRasterPos3i"));
    RasterPos3fv = reinterpret_cast<void (APIENTRY *)(const GLfloat *)>(resolveFunc("glRasterPos3fv"));
    RasterPos3f = reinterpret_cast<void (APIENTRY *)(GLfloat , GLfloat , GLfloat )>(resolveFunc("glRasterPos3f"));
    RasterPos3dv = reinterpret_cast<void (APIENTRY *)(const GLdouble *)>(resolveFunc("glRasterPos3dv"));
    RasterPos3d = reinterpret_cast<void (APIENTRY *)(GLdouble , GLdouble , GLdouble )>(resolveFunc("glRasterPos3d"));
    RasterPos2sv = reinterpret_cast<void (APIENTRY *)(const GLshort *)>(resolveFunc("glRasterPos2sv"));
    RasterPos2s = reinterpret_cast<void (APIENTRY *)(GLshort , GLshort )>(resolveFunc("glRasterPos2s"));
    RasterPos2iv = reinterpret_cast<void (APIENTRY *)(const GLint *)>(resolveFunc("glRasterPos2iv"));
    RasterPos2i = reinterpret_cast<void (APIENTRY *)(GLint , GLint )>(resolveFunc("glRasterPos2i"));
    RasterPos2fv = reinterpret_cast<void (APIENTRY *)(const GLfloat *)>(resolveFunc("glRasterPos2fv"));
    RasterPos2f = reinterpret_cast<void (APIENTRY *)(GLfloat , GLfloat )>(resolveFunc("glRasterPos2f"));
    RasterPos2dv = reinterpret_cast<void (APIENTRY *)(const GLdouble *)>(resolveFunc("glRasterPos2dv"));
    RasterPos2d = reinterpret_cast<void (APIENTRY *)(GLdouble , GLdouble )>(resolveFunc("glRasterPos2d"));
    Normal3sv = reinterpret_cast<void (APIENTRY *)(const GLshort *)>(resolveFunc("glNormal3sv"));
    Normal3s = reinterpret_cast<void (APIENTRY *)(GLshort , GLshort , GLshort )>(resolveFunc("glNormal3s"));
    Normal3iv = reinterpret_cast<void (APIENTRY *)(const GLint *)>(resolveFunc("glNormal3iv"));
    Normal3i = reinterpret_cast<void (APIENTRY *)(GLint , GLint , GLint )>(resolveFunc("glNormal3i"));
    Normal3fv = reinterpret_cast<void (APIENTRY *)(const GLfloat *)>(resolveFunc("glNormal3fv"));
    Normal3f = reinterpret_cast<void (APIENTRY *)(GLfloat , GLfloat , GLfloat )>(resolveFunc("glNormal3f"));
    Normal3dv = reinterpret_cast<void (APIENTRY *)(const GLdouble *)>(resolveFunc("glNormal3dv"));
    Normal3d = reinterpret_cast<void (APIENTRY *)(GLdouble , GLdouble , GLdouble )>(resolveFunc("glNormal3d"));
    Normal3bv = reinterpret_cast<void (APIENTRY *)(const GLbyte *)>(resolveFunc("glNormal3bv"));
    Normal3b = reinterpret_cast<void (APIENTRY *)(GLbyte , GLbyte , GLbyte )>(resolveFunc("glNormal3b"));
    Indexsv = reinterpret_cast<void (APIENTRY *)(const GLshort *)>(resolveFunc("glIndexsv"));
    Indexs = reinterpret_cast<void (APIENTRY *)(GLshort )>(resolveFunc("glIndexs"));
    Indexiv = reinterpret_cast<void (APIENTRY *)(const GLint *)>(resolveFunc("glIndexiv"));
    Indexi = reinterpret_cast<void (APIENTRY *)(GLint )>(resolveFunc("glIndexi"));
    Indexfv = reinterpret_cast<void (APIENTRY *)(const GLfloat *)>(resolveFunc("glIndexfv"));
    Indexf = reinterpret_cast<void (APIENTRY *)(GLfloat )>(resolveFunc("glIndexf"));
    Indexdv = reinterpret_cast<void (APIENTRY *)(const GLdouble *)>(resolveFunc("glIndexdv"));
    Indexd = reinterpret_cast<void (APIENTRY *)(GLdouble )>(resolveFunc("glIndexd"));
    End = reinterpret_cast<void (APIENTRY *)()>(resolveFunc("glEnd"));
    EdgeFlagv = reinterpret_cast<void (APIENTRY *)(const GLboolean *)>(resolveFunc("glEdgeFlagv"));
    EdgeFlag = reinterpret_cast<void (APIENTRY *)(GLboolean )>(resolveFunc("glEdgeFlag"));
    Color4usv = reinterpret_cast<void (APIENTRY *)(const GLushort *)>(resolveFunc("glColor4usv"));
    Color4us = reinterpret_cast<void (APIENTRY *)(GLushort , GLushort , GLushort , GLushort )>(resolveFunc("glColor4us"));
    Color4uiv = reinterpret_cast<void (APIENTRY *)(const GLuint *)>(resolveFunc("glColor4uiv"));
    Color4ui = reinterpret_cast<void (APIENTRY *)(GLuint , GLuint , GLuint , GLuint )>(resolveFunc("glColor4ui"));
    Color4ubv = reinterpret_cast<void (APIENTRY *)(const GLubyte *)>(resolveFunc("glColor4ubv"));
    Color4ub = reinterpret_cast<void (APIENTRY *)(GLubyte , GLubyte , GLubyte , GLubyte )>(resolveFunc("glColor4ub"));
    Color4sv = reinterpret_cast<void (APIENTRY *)(const GLshort *)>(resolveFunc("glColor4sv"));
    Color4s = reinterpret_cast<void (APIENTRY *)(GLshort , GLshort , GLshort , GLshort )>(resolveFunc("glColor4s"));
    Color4iv = reinterpret_cast<void (APIENTRY *)(const GLint *)>(resolveFunc("glColor4iv"));
    Color4i = reinterpret_cast<void (APIENTRY *)(GLint , GLint , GLint , GLint )>(resolveFunc("glColor4i"));
    Color4fv = reinterpret_cast<void (APIENTRY *)(const GLfloat *)>(resolveFunc("glColor4fv"));
    Color4f = reinterpret_cast<void (APIENTRY *)(GLfloat , GLfloat , GLfloat , GLfloat )>(resolveFunc("glColor4f"));
    Color4dv = reinterpret_cast<void (APIENTRY *)(const GLdouble *)>(resolveFunc("glColor4dv"));
    Color4d = reinterpret_cast<void (APIENTRY *)(GLdouble , GLdouble , GLdouble , GLdouble )>(resolveFunc("glColor4d"));
    Color4bv = reinterpret_cast<void (APIENTRY *)(const GLbyte *)>(resolveFunc("glColor4bv"));
    Color4b = reinterpret_cast<void (APIENTRY *)(GLbyte , GLbyte , GLbyte , GLbyte )>(resolveFunc("glColor4b"));
    Color3usv = reinterpret_cast<void (APIENTRY *)(const GLushort *)>(resolveFunc("glColor3usv"));
    Color3us = reinterpret_cast<void (APIENTRY *)(GLushort , GLushort , GLushort )>(resolveFunc("glColor3us"));
    Color3uiv = reinterpret_cast<void (APIENTRY *)(const GLuint *)>(resolveFunc("glColor3uiv"));
    Color3ui = reinterpret_cast<void (APIENTRY *)(GLuint , GLuint , GLuint )>(resolveFunc("glColor3ui"));
    Color3ubv = reinterpret_cast<void (APIENTRY *)(const GLubyte *)>(resolveFunc("glColor3ubv"));
    Color3ub = reinterpret_cast<void (APIENTRY *)(GLubyte , GLubyte , GLubyte )>(resolveFunc("glColor3ub"));
    Color3sv = reinterpret_cast<void (APIENTRY *)(const GLshort *)>(resolveFunc("glColor3sv"));
    Color3s = reinterpret_cast<void (APIENTRY *)(GLshort , GLshort , GLshort )>(resolveFunc("glColor3s"));
    Color3iv = reinterpret_cast<void (APIENTRY *)(const GLint *)>(resolveFunc("glColor3iv"));
    Color3i = reinterpret_cast<void (APIENTRY *)(GLint , GLint , GLint )>(resolveFunc("glColor3i"));
    Color3fv = reinterpret_cast<void (APIENTRY *)(const GLfloat *)>(resolveFunc("glColor3fv"));
    Color3f = reinterpret_cast<void (APIENTRY *)(GLfloat , GLfloat , GLfloat )>(resolveFunc("glColor3f"));
    Color3dv = reinterpret_cast<void (APIENTRY *)(const GLdouble *)>(resolveFunc("glColor3dv"));
    Color3d = reinterpret_cast<void (APIENTRY *)(GLdouble , GLdouble , GLdouble )>(resolveFunc("glColor3d"));
    Color3bv = reinterpret_cast<void (APIENTRY *)(const GLbyte *)>(resolveFunc("glColor3bv"));
    Color3b = reinterpret_cast<void (APIENTRY *)(GLbyte , GLbyte , GLbyte )>(resolveFunc("glColor3b"));
    Bitmap = reinterpret_cast<void (APIENTRY *)(GLsizei , GLsizei , GLfloat , GLfloat , GLfloat , GLfloat , const GLubyte *)>(resolveFunc("glBitmap"));
    Begin = reinterpret_cast<void (APIENTRY *)(GLenum )>(resolveFunc("glBegin"));
    ListBase = reinterpret_cast<void (APIENTRY *)(GLuint )>(resolveFunc("glListBase"));
    GenLists = reinterpret_cast<GLuint (APIENTRY *)(GLsizei )>(resolveFunc("glGenLists"));
    DeleteLists = reinterpret_cast<void (APIENTRY *)(GLuint , GLsizei )>(resolveFunc("glDeleteLists"));
    CallLists = reinterpret_cast<void (APIENTRY *)(GLsizei , GLenum , const GLvoid *)>(resolveFunc("glCallLists"));
    CallList = reinterpret_cast<void (APIENTRY *)(GLuint )>(resolveFunc("glCallList"));
    EndList = reinterpret_cast<void (APIENTRY *)()>(resolveFunc("glEndList"));
    NewList = reinterpret_cast<void (APIENTRY *)(GLuint , GLenum )>(resolveFunc("glNewList"));

    Indexubv = reinterpret_cast<void (APIENTRY *)(const GLubyte *)>(resolveFunc("glIndexubv"));
    Indexub = reinterpret_cast<void (APIENTRY *)(GLubyte )>(resolveFunc("glIndexub"));
    TexSubImage1D = reinterpret_cast<void (APIENTRY *)(GLenum , GLint , GLint , GLsizei , GLenum , GLenum , const GLvoid *)>(resolveFunc("glTexSubImage1D"));
    CopyTexSubImage1D = reinterpret_cast<void (APIENTRY *)(GLenum , GLint , GLint , GLint , GLint , GLsizei )>(resolveFunc("glCopyTexSubImage1D"));
    CopyTexImage1D = reinterpret_cast<void (APIENTRY *)(GLenum , GLint , GLenum , GLint , GLint , GLsizei , GLint )>(resolveFunc("glCopyTexImage1D"));
    GetPointerv = reinterpret_cast<void (APIENTRY *)(GLenum , GLvoid* *)>(resolveFunc("glGetPointerv"));

    PushClientAttrib = reinterpret_cast<void (APIENTRY *)(GLbitfield )>(resolveFunc("glPushClientAttrib"));
    PopClientAttrib = reinterpret_cast<void (APIENTRY *)()>(resolveFunc("glPopClientAttrib"));
    PrioritizeTextures = reinterpret_cast<void (APIENTRY *)(GLsizei , const GLuint *, const GLfloat *)>(resolveFunc("glPrioritizeTextures"));
    AreTexturesResident = reinterpret_cast<GLboolean (APIENTRY *)(GLsizei , const GLuint *, GLboolean *)>(resolveFunc("glAreTexturesResident"));
    VertexPointer = reinterpret_cast<void (APIENTRY *)(GLint , GLenum , GLsizei , const GLvoid *)>(resolveFunc("glVertexPointer"));
    TexCoordPointer = reinterpret_cast<void (APIENTRY *)(GLint , GLenum , GLsizei , const GLvoid *)>(resolveFunc("glTexCoordPointer"));
    NormalPointer = reinterpret_cast<void (APIENTRY *)(GLenum , GLsizei , const GLvoid *)>(resolveFunc("glNormalPointer"));
    InterleavedArrays = reinterpret_cast<void (APIENTRY *)(GLenum , GLsizei , const GLvoid *)>(resolveFunc("glInterleavedArrays"));
    IndexPointer = reinterpret_cast<void (APIENTRY *)(GLenum , GLsizei , const GLvoid *)>(resolveFunc("glIndexPointer"));
    EnableClientState = reinterpret_cast<void (APIENTRY *)(GLenum )>(resolveFunc("glEnableClientState"));
    EdgeFlagPointer = reinterpret_cast<void (APIENTRY *)(GLsizei , const GLvoid *)>(resolveFunc("glEdgeFlagPointer"));
    DisableClientState = reinterpret_cast<void (APIENTRY *)(GLenum )>(resolveFunc("glDisableClientState"));
    ColorPointer = reinterpret_cast<void (APIENTRY *)(GLint , GLenum , GLsizei , const GLvoid *)>(resolveFunc("glColorPointer"));
    ArrayElement = reinterpret_cast<void (APIENTRY *)(GLint )>(resolveFunc("glArrayElement"));
}

void QWindowsOpenGL::resolveGLES2()
{
    ActiveTexture = reinterpret_cast<void (APIENTRY *)(GLenum)>(resolveFunc("glActiveTexture"));
    AttachShader = reinterpret_cast<void (APIENTRY *)(GLuint , GLuint )>(resolveFunc("glAttachShader"));
    BindAttribLocation = reinterpret_cast<void (APIENTRY *)(GLuint , GLuint , const GLchar* )>(resolveFunc("glBindAttribLocation"));
    BindBuffer = reinterpret_cast<void (APIENTRY *)(GLenum , GLuint )>(resolveFunc("glBindBuffer"));
    BindFramebuffer = reinterpret_cast<void (APIENTRY *)(GLenum , GLuint )>(resolveFunc("glBindFramebuffer"));
    BindRenderbuffer = reinterpret_cast<void (APIENTRY *)(GLenum , GLuint )>(resolveFunc("glBindRenderbuffer"));
    BlendColor = reinterpret_cast<void (APIENTRY *)(GLclampf , GLclampf , GLclampf , GLclampf )>(resolveFunc("glBlendColor"));
    BlendEquation = reinterpret_cast<void (APIENTRY *)(GLenum )>(resolveFunc("glBlendEquation"));
    BlendEquationSeparate = reinterpret_cast<void (APIENTRY *)(GLenum , GLenum )>(resolveFunc("glBlendEquationSeparate"));
    BlendFuncSeparate = reinterpret_cast<void (APIENTRY *)(GLenum , GLenum , GLenum , GLenum )>(resolveFunc("glBlendFuncSeparate"));
    BufferData = reinterpret_cast<void (APIENTRY *)(GLenum , GLsizeiptr , const GLvoid* , GLenum )>(resolveFunc("glBufferData"));
    BufferSubData = reinterpret_cast<void (APIENTRY *)(GLenum , GLintptr , GLsizeiptr , const GLvoid* )>(resolveFunc("glBufferSubData"));
    CheckFramebufferStatus = reinterpret_cast<GLenum (APIENTRY *)(GLenum )>(resolveFunc("glCheckFramebufferStatus"));
    ClearDepthf = reinterpret_cast<void (APIENTRY *)(GLclampf )>(resolveFunc("glClearDepthf"));
    CompileShader = reinterpret_cast<void (APIENTRY *)(GLuint )>(resolveFunc("glCompileShader"));
    CompressedTexImage2D = reinterpret_cast<void (APIENTRY *)(GLenum , GLint , GLenum , GLsizei , GLsizei, GLint, GLsizei, const GLvoid* )>(resolveFunc("glCompressedTexImage2D"));
    CompressedTexSubImage2D = reinterpret_cast<void (APIENTRY *)(GLenum , GLint , GLint , GLint, GLsizei, GLsizei, GLenum, GLsizei, const GLvoid* )>(resolveFunc("glCompressedTexSubImage2D"));
    CreateProgram = reinterpret_cast<GLuint (APIENTRY *)(void)>(resolveFunc("glCreateProgram"));
    CreateShader = reinterpret_cast<GLuint (APIENTRY *)(GLenum )>(resolveFunc("glCreateShader"));
    DeleteBuffers = reinterpret_cast<void (APIENTRY *)(GLsizei , const GLuint*)>(resolveFunc("glDeleteBuffers"));
    DeleteFramebuffers = reinterpret_cast<void (APIENTRY *)(GLsizei , const GLuint* )>(resolveFunc("glDeleteFramebuffers"));
    DeleteProgram = reinterpret_cast<void (APIENTRY *)(GLuint )>(resolveFunc("glDeleteProgram"));
    DeleteRenderbuffers = reinterpret_cast<void (APIENTRY *)(GLsizei , const GLuint* )>(resolveFunc("glDeleteRenderbuffers"));
    DeleteShader = reinterpret_cast<void (APIENTRY *)(GLuint )>(resolveFunc("glDeleteShader"));
    DepthRangef = reinterpret_cast<void (APIENTRY *)(GLclampf , GLclampf )>(resolveFunc("glDepthRangef"));
    DetachShader = reinterpret_cast<void (APIENTRY *)(GLuint , GLuint )>(resolveFunc("glDetachShader"));
    DisableVertexAttribArray = reinterpret_cast<void (APIENTRY *)(GLuint )>(resolveFunc("glDisableVertexAttribArray"));
    EnableVertexAttribArray = reinterpret_cast<void (APIENTRY *)(GLuint )>(resolveFunc("glEnableVertexAttribArray"));
    FramebufferRenderbuffer = reinterpret_cast<void (APIENTRY *)(GLenum , GLenum , GLenum , GLuint )>(resolveFunc("glFramebufferRenderbuffer"));
    FramebufferTexture2D = reinterpret_cast<void (APIENTRY *)(GLenum , GLenum , GLenum , GLuint , GLint )>(resolveFunc("glFramebufferTexture2D"));
    GenBuffers = reinterpret_cast<void (APIENTRY *)(GLsizei , GLuint* )>(resolveFunc("glGenBuffers"));
    GenerateMipmap = reinterpret_cast<void (APIENTRY *)(GLenum )>(resolveFunc("glGenerateMipmap"));
    GenFramebuffers = reinterpret_cast<void (APIENTRY *)(GLsizei , GLuint* )>(resolveFunc("glGenFramebuffers"));
    GenRenderbuffers = reinterpret_cast<void (APIENTRY *)(GLsizei , GLuint* )>(resolveFunc("glGenRenderbuffers"));
    GetActiveAttrib = reinterpret_cast<void (APIENTRY *)(GLuint , GLuint , GLsizei , GLsizei* , GLint* , GLenum* , GLchar* )>(resolveFunc("glGetActiveAttrib"));
    GetActiveUniform = reinterpret_cast<void (APIENTRY *)(GLuint , GLuint , GLsizei , GLsizei* , GLint* , GLenum* , GLchar* )>(resolveFunc("glGetActiveUniform"));
    GetAttachedShaders = reinterpret_cast<void (APIENTRY *)(GLuint , GLsizei , GLsizei*, GLuint* )>(resolveFunc("glGetAttachedShaders"));
    GetAttribLocation = reinterpret_cast<int (APIENTRY *)(GLuint , const GLchar* )>(resolveFunc("glGetAttribLocation"));
    GetBufferParameteriv = reinterpret_cast<void (APIENTRY *)(GLenum , GLenum , GLint* )>(resolveFunc("glGetBufferParameteriv"));
    GetFramebufferAttachmentParameteriv = reinterpret_cast<void (APIENTRY *)(GLenum , GLenum, GLenum , GLint* )>(resolveFunc("glGetFramebufferAttachmentParameteriv"));
    GetProgramiv = reinterpret_cast<void (APIENTRY *)(GLuint , GLenum , GLint* )>(resolveFunc("glGetProgramiv"));
    GetProgramInfoLog = reinterpret_cast<void (APIENTRY *)(GLuint , GLsizei , GLsizei* , GLchar* )>(resolveFunc("glGetProgramInfoLog"));
    GetRenderbufferParameteriv = reinterpret_cast<void (APIENTRY *)(GLenum , GLenum , GLint* )>(resolveFunc("glGetRenderbufferParameteriv"));
    GetShaderiv = reinterpret_cast<void (APIENTRY *)(GLuint , GLenum , GLint* )>(resolveFunc("glGetShaderiv"));
    GetShaderInfoLog = reinterpret_cast<void (APIENTRY *)(GLuint , GLsizei , GLsizei*, GLchar*)>(resolveFunc("glGetShaderInfoLog"));
    GetShaderPrecisionFormat = reinterpret_cast<void (APIENTRY *)(GLenum , GLenum , GLint* , GLint* )>(resolveFunc("glGetShaderPrecisionFormat"));
    GetShaderSource = reinterpret_cast<void (APIENTRY *)(GLuint , GLsizei , GLsizei* , GLchar* )>(resolveFunc("glGetShaderSource"));
    GetUniformfv = reinterpret_cast<void (APIENTRY *)(GLuint , GLint , GLfloat*)>(resolveFunc("glGetUniformfv"));
    GetUniformiv = reinterpret_cast<void (APIENTRY *)(GLuint , GLint , GLint*)>(resolveFunc("glGetUniformiv"));
    GetUniformLocation = reinterpret_cast<int (APIENTRY *)(GLuint , const GLchar* )>(resolveFunc("glGetUniformLocation"));
    GetVertexAttribfv = reinterpret_cast<void (APIENTRY *)(GLuint , GLenum , GLfloat* )>(resolveFunc("glGetVertexAttribfv"));
    GetVertexAttribiv = reinterpret_cast<void (APIENTRY *)(GLuint , GLenum , GLint* )>(resolveFunc("glGetVertexAttribiv"));
    GetVertexAttribPointerv = reinterpret_cast<void (APIENTRY *)(GLuint , GLenum , GLvoid** pointer)>(resolveFunc("glGetVertexAttribPointerv"));
    IsBuffer = reinterpret_cast<GLboolean (APIENTRY *)(GLuint )>(resolveFunc("glIsBuffer"));
    IsFramebuffer = reinterpret_cast<GLboolean (APIENTRY *)(GLuint )>(resolveFunc("glIsFramebuffer"));
    IsProgram = reinterpret_cast<GLboolean (APIENTRY *)(GLuint )>(resolveFunc("glIsProgram"));
    IsRenderbuffer = reinterpret_cast<GLboolean (APIENTRY *)(GLuint )>(resolveFunc("glIsRenderbuffer"));
    IsShader = reinterpret_cast<GLboolean (APIENTRY *)(GLuint )>(resolveFunc("glIsShader"));
    LinkProgram = reinterpret_cast<void (APIENTRY *)(GLuint )>(resolveFunc("glLinkProgram"));
    ReleaseShaderCompiler = reinterpret_cast<void (APIENTRY *)(void)>(resolveFunc("glReleaseShaderCompiler"));
    RenderbufferStorage = reinterpret_cast<void (APIENTRY *)(GLenum , GLenum , GLsizei , GLsizei )>(resolveFunc("glRenderbufferStorage"));
    SampleCoverage = reinterpret_cast<void (APIENTRY *)(GLclampf , GLboolean )>(resolveFunc("glSampleCoverage"));
    ShaderBinary = reinterpret_cast<void (APIENTRY *)(GLsizei , const GLuint*, GLenum , const GLvoid* , GLsizei )>(resolveFunc("glShaderBinary"));
    ShaderSource = reinterpret_cast<void (APIENTRY *)(GLuint , GLsizei , const GLchar* *, const GLint* )>(resolveFunc("glShaderSource"));
    StencilFuncSeparate = reinterpret_cast<void (APIENTRY *)(GLenum , GLenum , GLint , GLuint )>(resolveFunc("glStencilFuncSeparate"));
    StencilMaskSeparate = reinterpret_cast<void (APIENTRY *)(GLenum , GLuint )>(resolveFunc("glStencilMaskSeparate"));
    StencilOpSeparate = reinterpret_cast<void (APIENTRY *)(GLenum , GLenum , GLenum , GLenum )>(resolveFunc("glStencilOpSeparate"));
    Uniform1f = reinterpret_cast<void (APIENTRY *)(GLint , GLfloat )>(resolveFunc("glUniform1f"));
    Uniform1fv = reinterpret_cast<void (APIENTRY *)(GLint , GLsizei , const GLfloat* )>(resolveFunc("glUniform1fv"));
    Uniform1i = reinterpret_cast<void (APIENTRY *)(GLint , GLint )>(resolveFunc("glUniform1i"));
    Uniform1iv = reinterpret_cast<void (APIENTRY *)(GLint , GLsizei , const GLint* )>(resolveFunc("glUniform1iv"));
    Uniform2f = reinterpret_cast<void (APIENTRY *)(GLint , GLfloat , GLfloat )>(resolveFunc("glUniform2f"));
    Uniform2fv = reinterpret_cast<void (APIENTRY *)(GLint , GLsizei , const GLfloat* )>(resolveFunc("glUniform2fv"));
    Uniform2i = reinterpret_cast<void (APIENTRY *)(GLint , GLint , GLint )>(resolveFunc("glUniform2i"));
    Uniform2iv = reinterpret_cast<void (APIENTRY *)(GLint , GLsizei , const GLint* )>(resolveFunc("glUniform2iv"));
    Uniform3f = reinterpret_cast<void (APIENTRY *)(GLint , GLfloat , GLfloat , GLfloat )>(resolveFunc("glUniform3f"));
    Uniform3fv = reinterpret_cast<void (APIENTRY *)(GLint , GLsizei , const GLfloat* )>(resolveFunc("glUniform3fv"));
    Uniform3i = reinterpret_cast<void (APIENTRY *)(GLint , GLint , GLint , GLint )>(resolveFunc("glUniform3i"));
    Uniform3iv = reinterpret_cast<void (APIENTRY *)(GLint , GLsizei , const GLint* )>(resolveFunc("glUniform3iv"));
    Uniform4f = reinterpret_cast<void (APIENTRY *)(GLint , GLfloat , GLfloat , GLfloat , GLfloat )>(resolveFunc("glUniform4f"));
    Uniform4fv = reinterpret_cast<void (APIENTRY *)(GLint , GLsizei , const GLfloat* )>(resolveFunc("glUniform4fv"));
    Uniform4i = reinterpret_cast<void (APIENTRY *)(GLint , GLint , GLint , GLint , GLint )>(resolveFunc("glUniform4i"));
    Uniform4iv = reinterpret_cast<void (APIENTRY *)(GLint , GLsizei , const GLint* )>(resolveFunc("glUniform4iv"));
    UniformMatrix2fv = reinterpret_cast<void (APIENTRY *)(GLint , GLsizei , GLboolean , const GLfloat* )>(resolveFunc("glUniformMatrix2fv"));
    UniformMatrix3fv = reinterpret_cast<void (APIENTRY *)(GLint , GLsizei , GLboolean , const GLfloat* )>(resolveFunc("glUniformMatrix3fv"));
    UniformMatrix4fv = reinterpret_cast<void (APIENTRY *)(GLint , GLsizei , GLboolean , const GLfloat* )>(resolveFunc("glUniformMatrix4fv"));
    UseProgram = reinterpret_cast<void (APIENTRY *)(GLuint )>(resolveFunc("glUseProgram"));
    ValidateProgram = reinterpret_cast<void (APIENTRY *)(GLuint )>(resolveFunc("glValidateProgram"));
    VertexAttrib1f = reinterpret_cast<void (APIENTRY *)(GLuint , GLfloat )>(resolveFunc("glVertexAttrib1f"));
    VertexAttrib1fv = reinterpret_cast<void (APIENTRY *)(GLuint , const GLfloat* )>(resolveFunc("glVertexAttrib1fv"));
    VertexAttrib2f = reinterpret_cast<void (APIENTRY *)(GLuint , GLfloat , GLfloat )>(resolveFunc("glVertexAttrib2f"));
    VertexAttrib2fv = reinterpret_cast<void (APIENTRY *)(GLuint , const GLfloat* )>(resolveFunc("glVertexAttrib2fv"));
    VertexAttrib3f = reinterpret_cast<void (APIENTRY *)(GLuint , GLfloat , GLfloat , GLfloat )>(resolveFunc("glVertexAttrib3f"));
    VertexAttrib3fv = reinterpret_cast<void (APIENTRY *)(GLuint , const GLfloat* )>(resolveFunc("glVertexAttrib3fv"));
    VertexAttrib4f = reinterpret_cast<void (APIENTRY *)(GLuint , GLfloat , GLfloat , GLfloat , GLfloat )>(resolveFunc("glVertexAttrib4f"));
    VertexAttrib4fv = reinterpret_cast<void (APIENTRY *)(GLuint , const GLfloat* )>(resolveFunc("glVertexAttrib4fv"));
    VertexAttribPointer = reinterpret_cast<void (APIENTRY *)(GLuint , GLint, GLenum, GLboolean, GLsizei, const GLvoid* )>(resolveFunc("glVertexAttribPointer"));
}

void QWindowsOpenGL::resolve()
{
    switch (libraryType()) {
    case DesktopGL:
        resolveWGL();
        resolveGLCommon();
        resolveGL11();
        break;

    case GLES2:
        resolveEGL();
        resolveGLCommon();
        resolveGLES2();
        break;

    default:
        Q_ASSERT_X(0, "QWindowsOpenGL", "Nothing to resolve");
        break;
    }
}

bool QWindowsOpenGL::testDesktopGL()
{
    HMODULE lib = 0;
    HWND wnd = 0;
    HDC dc = 0;
    HGLRC context = 0;
    LPCTSTR className = L"qtopenglproxytest";

    HGLRC (WINAPI * CreateContext)(HDC dc) = 0;
    BOOL (WINAPI * DeleteContext)(HGLRC context) = 0;
    BOOL (WINAPI * MakeCurrent)(HDC dc, HGLRC context) = 0;
    PROC (WINAPI * WGL_GetProcAddress)(LPCSTR name) = 0;

    bool result = false;

    // Test #1: Load opengl32.dll and try to resolve an OpenGL 2 function.
    // This will typically fail on systems that do not have a real OpenGL driver.
    lib = qgl_loadLib("opengl32.dll", false);
    if (lib) {
        CreateContext = reinterpret_cast<HGLRC (WINAPI *)(HDC)>(::GetProcAddress(lib, "wglCreateContext"));
        if (!CreateContext)
            goto cleanup;
        DeleteContext = reinterpret_cast<BOOL (WINAPI *)(HGLRC)>(::GetProcAddress(lib, "wglDeleteContext"));
        if (!DeleteContext)
            goto cleanup;
        MakeCurrent = reinterpret_cast<BOOL (WINAPI *)(HDC, HGLRC)>(::GetProcAddress(lib, "wglMakeCurrent"));
        if (!MakeCurrent)
            goto cleanup;
        WGL_GetProcAddress = reinterpret_cast<PROC (WINAPI *)(LPCSTR)>(::GetProcAddress(lib, "wglGetProcAddress"));
        if (!WGL_GetProcAddress)
            goto cleanup;

        WNDCLASS wclass;
        wclass.cbClsExtra = 0;
        wclass.cbWndExtra = 0;
        wclass.hInstance = (HINSTANCE) GetModuleHandle(0);
        wclass.hIcon = 0;
        wclass.hCursor = 0;
        wclass.hbrBackground = (HBRUSH) (COLOR_BACKGROUND);
        wclass.lpszMenuName = 0;
        wclass.lpfnWndProc = DefWindowProc;
        wclass.lpszClassName = className;
        wclass.style = CS_OWNDC;
        if (!RegisterClass(&wclass))
            goto cleanup;
        wnd = CreateWindow(className, L"qtopenglproxytest", WS_OVERLAPPED,
                           0, 0, 640, 480, 0, 0, wclass.hInstance, 0);
        if (!wnd)
            goto cleanup;
        dc = GetDC(wnd);
        if (!dc)
            goto cleanup;

        PIXELFORMATDESCRIPTOR pfd;
        memset(&pfd, 0, sizeof(PIXELFORMATDESCRIPTOR));
        pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
        pfd.nVersion = 1;
        pfd.dwFlags = PFD_SUPPORT_OPENGL | PFD_DRAW_TO_WINDOW | PFD_GENERIC_FORMAT;
        pfd.iPixelType = PFD_TYPE_RGBA;
        // Use the GDI functions. Under the hood this will call the wgl variants in opengl32.dll.
        int pixelFormat = ChoosePixelFormat(dc, &pfd);
        if (!pixelFormat)
            goto cleanup;
        if (!SetPixelFormat(dc, pixelFormat, &pfd))
            goto cleanup;
        context = CreateContext(dc);
        if (!context)
            goto cleanup;
        if (!MakeCurrent(dc, context))
            goto cleanup;

        // Now that there is finally a context current, try doing something useful.
        if (WGL_GetProcAddress("glCreateShader")) {
            result = true;
            qCDebug(qglLc, "OpenGL 2 entry points available");
        } else {
            qCDebug(qglLc, "OpenGL 2 entry points not found");
        }
    } else {
        qCDebug(qglLc, "Failed to load opengl32.dll");
    }

cleanup:
    if (MakeCurrent)
        MakeCurrent(0, 0);
    if (context)
        DeleteContext(context);
    if (dc && wnd)
        ReleaseDC(wnd, dc);
    if (wnd)
        DestroyWindow(wnd);
    UnregisterClass(className, GetModuleHandle(0));
    if (lib)
        FreeLibrary(lib);

    return result;
}

class QWindowsOpenGLList
{
public:
    QWindowsOpenGLList();
    ~QWindowsOpenGLList();
    QVector<QAbstractWindowsOpenGL *> list;
};

QWindowsOpenGLList::QWindowsOpenGLList()
{
    // For now there is always one OpenGL ( + winsys interface) loaded.
    // This may change in the future.
    list.append(new QWindowsOpenGL);
}

QWindowsOpenGLList::~QWindowsOpenGLList()
{
    qDeleteAll(list);
}

// Use Q_GLOBAL_STATIC and perform initialization in the constructor to be
// thread safe.
Q_GLOBAL_STATIC(QWindowsOpenGLList, gl)

static inline QAbstractWindowsOpenGL *qgl_choose()
{
    return gl()->list[0];
}

// functionsReady() -> the DLL is there but some functions were not resolved. This is fatal.
// !functionsReady() -> could not load a GL implementation. No error message in this case.
#define GLWARN(g, func, prefix)                                         \
    {                                                                   \
        if (g->functionsReady())                                        \
            qFatal("Qt OpenGL: Attempted to call unresolved function %s%s. " \
                   "This is likely caused by making OpenGL-only calls with an OpenGL ES implementation (Angle).", \
                   prefix, #func);                                      \
    }

#define GLCALLV(func, ...)                              \
    {                                                   \
        QAbstractWindowsOpenGL *g = qgl_choose();       \
        if (g->func)                                    \
            g->func(__VA_ARGS__);                       \
        else                                            \
            GLWARN(g, func, "gl")                       \
    }

#define GLCALL(func, ...)                               \
    {                                                   \
        QAbstractWindowsOpenGL *g = qgl_choose();       \
        if (g->func)                                    \
            return g->func(__VA_ARGS__);                \
        GLWARN(g, func, "gl")                           \
        return 0;                                       \
    }

#define WGLCALL(func, ...)                              \
    {                                                   \
        QAbstractWindowsOpenGL *g = qgl_choose();       \
        if (g->func)                                    \
            return g->func(__VA_ARGS__);                \
        GLWARN(g, func, "wgl")                          \
        return 0;                                       \
    }

#define EGLCALL(func, ...)                              \
    {                                                   \
        QAbstractWindowsOpenGL *g = qgl_choose();       \
        if (g->EGL_##func)                              \
            return g->EGL_##func(__VA_ARGS__);          \
        GLWARN(g, func, "egl")                          \
        return 0;                                       \
    }


extern "C" {

// WGL

Q_DECL_EXPORT BOOL WINAPI wglCopyContext(HGLRC src, HGLRC dst, UINT mask)
{
    WGLCALL(CopyContext, src, dst, mask);
}

Q_DECL_EXPORT HGLRC WINAPI wglCreateContext(HDC dc)
{
    WGLCALL(CreateContext, dc);
}

Q_DECL_EXPORT HGLRC WINAPI wglCreateLayerContext(HDC dc, int plane)
{
    WGLCALL(CreateLayerContext, dc, plane);
}

Q_DECL_EXPORT BOOL WINAPI wglDeleteContext(HGLRC context)
{
    WGLCALL(DeleteContext, context);
}

Q_DECL_EXPORT HGLRC WINAPI wglGetCurrentContext(VOID)
{
    WGLCALL(GetCurrentContext);
}

Q_DECL_EXPORT HDC WINAPI wglGetCurrentDC(VOID)
{
    WGLCALL(GetCurrentDC);
}

Q_DECL_EXPORT PROC WINAPI wglGetProcAddress(LPCSTR name)
{
    WGLCALL(GetProcAddress, name);
}

Q_DECL_EXPORT BOOL WINAPI wglMakeCurrent(HDC dc, HGLRC context)
{
    WGLCALL(MakeCurrent, dc, context);
}

Q_DECL_EXPORT BOOL WINAPI wglShareLists(HGLRC context1, HGLRC context2)
{
    WGLCALL(ShareLists, context1, context2);
}

Q_DECL_EXPORT BOOL WINAPI wglUseFontBitmapsW(HDC dc, DWORD first, DWORD count, DWORD base)
{
    WGLCALL(UseFontBitmapsW, dc, first, count, base);
}

Q_DECL_EXPORT BOOL WINAPI wglUseFontOutlinesW(HDC dc, DWORD first, DWORD count, DWORD base, FLOAT deviation,
                                              FLOAT extrusion, int format, LPGLYPHMETRICSFLOAT gmf)
{
    WGLCALL(UseFontOutlinesW, dc, first, count, base, deviation, extrusion, format, gmf);
}

Q_DECL_EXPORT BOOL WINAPI wglDescribeLayerPlane(HDC dc, int pixelFormat, int plane, UINT n,
                                                LPLAYERPLANEDESCRIPTOR planeDescriptor)
{
    WGLCALL(DescribeLayerPlane, dc, pixelFormat, plane, n, planeDescriptor);
}

Q_DECL_EXPORT int WINAPI wglSetLayerPaletteEntries(HDC dc, int plane, int start, int entries,
                                                   CONST COLORREF *colors)
{
    WGLCALL(SetLayerPaletteEntries, dc, plane, start, entries, colors);
}

Q_DECL_EXPORT int WINAPI wglGetLayerPaletteEntries(HDC dc, int plane, int start, int entries,
                                                   COLORREF *color)
{
    WGLCALL(GetLayerPaletteEntries, dc, plane, start, entries, color);
}

Q_DECL_EXPORT BOOL WINAPI wglRealizeLayerPalette(HDC dc, int plane, BOOL realize)
{
    WGLCALL(RealizeLayerPalette, dc, plane, realize);
}

Q_DECL_EXPORT BOOL WINAPI wglSwapLayerBuffers(HDC dc, UINT planes)
{
    WGLCALL(SwapLayerBuffers, dc, planes);
}

Q_DECL_EXPORT DWORD WINAPI wglSwapMultipleBuffers(UINT n, CONST WGLSWAP *buffers)
{
    WGLCALL(SwapMultipleBuffers, n, buffers);
}

// EGL

Q_DECL_EXPORT EGLint EGLAPIENTRY eglGetError(void)
{
    EGLCALL(GetError);
}

Q_DECL_EXPORT EGLDisplay EGLAPIENTRY eglGetDisplay(EGLNativeDisplayType display_id)
{
    EGLCALL(GetDisplay, display_id);
}

Q_DECL_EXPORT EGLBoolean EGLAPIENTRY eglInitialize(EGLDisplay dpy, EGLint *major, EGLint *minor)
{
    EGLCALL(Initialize, dpy, major, minor);
}

Q_DECL_EXPORT EGLBoolean EGLAPIENTRY eglTerminate(EGLDisplay dpy)
{
    EGLCALL(Terminate, dpy);
}

Q_DECL_EXPORT const char * EGLAPIENTRY eglQueryString(EGLDisplay dpy, EGLint name)
{
    EGLCALL(QueryString, dpy, name);
}

Q_DECL_EXPORT EGLBoolean EGLAPIENTRY eglGetConfigs(EGLDisplay dpy, EGLConfig *configs,
                                                   EGLint config_size, EGLint *num_config)
{
    EGLCALL(GetConfigs, dpy, configs, config_size, num_config);
}

Q_DECL_EXPORT EGLBoolean EGLAPIENTRY eglChooseConfig(EGLDisplay dpy, const EGLint *attrib_list,
                                                     EGLConfig *configs, EGLint config_size,
                                                     EGLint *num_config)
{
    EGLCALL(ChooseConfig, dpy, attrib_list, configs, config_size, num_config);
}

Q_DECL_EXPORT EGLBoolean EGLAPIENTRY eglGetConfigAttrib(EGLDisplay dpy, EGLConfig config,
                                                        EGLint attribute, EGLint *value)
{
    EGLCALL(GetConfigAttrib, dpy, config, attribute, value);
}

Q_DECL_EXPORT EGLSurface EGLAPIENTRY eglCreateWindowSurface(EGLDisplay dpy, EGLConfig config,
                                                            EGLNativeWindowType win,
                                                            const EGLint *attrib_list)
{
    EGLCALL(CreateWindowSurface, dpy, config, win, attrib_list);
}

Q_DECL_EXPORT EGLSurface EGLAPIENTRY eglCreatePbufferSurface(EGLDisplay dpy, EGLConfig config,
                                                             const EGLint *attrib_list)
{
    EGLCALL(CreatePbufferSurface, dpy, config, attrib_list);
}

Q_DECL_EXPORT EGLSurface EGLAPIENTRY eglCreatePixmapSurface(EGLDisplay dpy, EGLConfig config,
                                                            EGLNativePixmapType pixmap,
                                                            const EGLint *attrib_list)
{
    EGLCALL(CreatePixmapSurface, dpy, config, pixmap, attrib_list);
}

Q_DECL_EXPORT EGLBoolean EGLAPIENTRY eglDestroySurface(EGLDisplay dpy, EGLSurface surface)
{
    EGLCALL(DestroySurface, dpy, surface);
}

Q_DECL_EXPORT EGLBoolean EGLAPIENTRY eglQuerySurface(EGLDisplay dpy, EGLSurface surface,
                                                     EGLint attribute, EGLint *value)
{
    EGLCALL(QuerySurface, dpy, surface, attribute, value);
}

Q_DECL_EXPORT EGLBoolean EGLAPIENTRY eglBindAPI(EGLenum api)
{
    EGLCALL(BindAPI, api);
}

Q_DECL_EXPORT EGLenum EGLAPIENTRY eglQueryAPI(void)
{
    EGLCALL(QueryAPI);
}

Q_DECL_EXPORT EGLBoolean EGLAPIENTRY eglWaitClient(void)
{
    EGLCALL(WaitClient);
}

Q_DECL_EXPORT EGLBoolean EGLAPIENTRY eglReleaseThread(void)
{
    EGLCALL(ReleaseThread);
}

Q_DECL_EXPORT EGLSurface EGLAPIENTRY eglCreatePbufferFromClientBuffer(
    EGLDisplay dpy, EGLenum buftype, EGLClientBuffer buffer,
    EGLConfig config, const EGLint *attrib_list)
{
    EGLCALL(CreatePbufferFromClientBuffer, dpy, buftype, buffer, config, attrib_list);
}

Q_DECL_EXPORT EGLBoolean EGLAPIENTRY eglSurfaceAttrib(EGLDisplay dpy, EGLSurface surface,
                                                      EGLint attribute, EGLint value)
{
    EGLCALL(SurfaceAttrib, dpy, surface, attribute, value);
}

Q_DECL_EXPORT EGLBoolean EGLAPIENTRY eglBindTexImage(EGLDisplay dpy, EGLSurface surface, EGLint buffer)
{
    EGLCALL(BindTexImage, dpy, surface, buffer);
}

Q_DECL_EXPORT EGLBoolean EGLAPIENTRY eglReleaseTexImage(EGLDisplay dpy, EGLSurface surface, EGLint buffer)
{
    EGLCALL(ReleaseTexImage, dpy, surface, buffer);
}

Q_DECL_EXPORT EGLBoolean EGLAPIENTRY eglSwapInterval(EGLDisplay dpy, EGLint interval)
{
    EGLCALL(SwapInterval, dpy, interval);
}

Q_DECL_EXPORT EGLContext EGLAPIENTRY eglCreateContext(EGLDisplay dpy, EGLConfig config,
                                                      EGLContext share_context,
                                                      const EGLint *attrib_list)
{
    EGLCALL(CreateContext, dpy, config, share_context, attrib_list);
}

Q_DECL_EXPORT EGLBoolean EGLAPIENTRY eglDestroyContext(EGLDisplay dpy, EGLContext ctx)
{
    EGLCALL(DestroyContext, dpy, ctx);
}

Q_DECL_EXPORT EGLBoolean EGLAPIENTRY eglMakeCurrent(EGLDisplay dpy, EGLSurface draw,
                                                    EGLSurface read, EGLContext ctx)
{
    EGLCALL(MakeCurrent, dpy, draw, read, ctx);
}

Q_DECL_EXPORT EGLContext EGLAPIENTRY eglGetCurrentContext(void)
{
    EGLCALL(GetCurrentContext);
}

Q_DECL_EXPORT EGLSurface EGLAPIENTRY eglGetCurrentSurface(EGLint readdraw)
{
    EGLCALL(GetCurrentSurface, readdraw);
}

Q_DECL_EXPORT EGLDisplay EGLAPIENTRY eglGetCurrentDisplay(void)
{
    EGLCALL(GetCurrentDisplay);
}

Q_DECL_EXPORT EGLBoolean EGLAPIENTRY eglQueryContext(EGLDisplay dpy, EGLContext ctx,
                                                     EGLint attribute, EGLint *value)
{
    EGLCALL(QueryContext, dpy, ctx, attribute, value);
}

Q_DECL_EXPORT EGLBoolean EGLAPIENTRY eglWaitGL(void)
{
    EGLCALL(WaitGL);
}

Q_DECL_EXPORT EGLBoolean EGLAPIENTRY eglWaitNative(EGLint engine)
{
    EGLCALL(WaitNative, engine);
}

Q_DECL_EXPORT EGLBoolean EGLAPIENTRY eglSwapBuffers(EGLDisplay dpy, EGLSurface surface)
{
    EGLCALL(SwapBuffers, dpy, surface);
}

Q_DECL_EXPORT EGLBoolean EGLAPIENTRY eglCopyBuffers(EGLDisplay dpy, EGLSurface surface,
                                                    EGLNativePixmapType target)
{
    EGLCALL(CopyBuffers, dpy, surface, target);
}

// OpenGL

Q_DECL_EXPORT void APIENTRY glViewport(GLint x, GLint y, GLsizei width, GLsizei height)
{
    GLCALLV(Viewport, x, y, width, height);
}

Q_DECL_EXPORT void APIENTRY glDepthRange(GLdouble nearVal, GLdouble farVal)
{
    if (qgl_choose()->libraryType() == QAbstractWindowsOpenGL::DesktopGL) {
        GLCALLV(DepthRange, nearVal, farVal);
    } else {
        GLCALLV(DepthRangef, nearVal, farVal);
    }
}

Q_DECL_EXPORT GLboolean APIENTRY glIsEnabled(GLenum cap)
{
    GLCALL(IsEnabled, cap);
}

Q_DECL_EXPORT void APIENTRY glGetTexLevelParameteriv(GLenum target, GLint level, GLenum pname, GLint *params)
{
    GLCALLV(GetTexLevelParameteriv, target, level, pname, params);
}

Q_DECL_EXPORT void APIENTRY glGetTexLevelParameterfv(GLenum target, GLint level, GLenum pname, GLfloat *params)
{
    GLCALLV(GetTexLevelParameterfv, target, level, pname, params);
}

Q_DECL_EXPORT void APIENTRY glGetTexParameteriv(GLenum target, GLenum pname, GLint *params)
{
    GLCALLV(GetTexParameteriv, target, pname, params);
}

Q_DECL_EXPORT void APIENTRY glGetTexParameterfv(GLenum target, GLenum pname, GLfloat *params)
{
    GLCALLV(GetTexParameterfv, target, pname, params);
}

Q_DECL_EXPORT void APIENTRY glGetTexImage(GLenum target, GLint level, GLenum format, GLenum type, GLvoid *pixels)
{
    GLCALLV(GetTexImage, target, level, format, type, pixels);
}

Q_DECL_EXPORT const GLubyte * APIENTRY glGetString(GLenum name)
{
    GLCALL(GetString, name);
}

Q_DECL_EXPORT void APIENTRY glGetIntegerv(GLenum pname, GLint *params)
{
    GLCALLV(GetIntegerv, pname, params);
}

Q_DECL_EXPORT void APIENTRY glGetFloatv(GLenum pname, GLfloat *params)
{
    GLCALLV(GetFloatv, pname, params);
}

Q_DECL_EXPORT GLenum APIENTRY glGetError()
{
    GLCALL(GetError);
}

Q_DECL_EXPORT void APIENTRY glGetDoublev(GLenum pname, GLdouble *params)
{
    GLCALLV(GetDoublev, pname, params);
}

Q_DECL_EXPORT void APIENTRY glGetBooleanv(GLenum pname, GLboolean *params)
{
    GLCALLV(GetBooleanv, pname, params);
}

Q_DECL_EXPORT void APIENTRY glReadPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid *pixels)
{
    GLCALLV(ReadPixels, x, y, width, height, format, type, pixels);
}

Q_DECL_EXPORT void APIENTRY glReadBuffer(GLenum mode)
{
    GLCALLV(ReadBuffer, mode);
}

Q_DECL_EXPORT void APIENTRY glPixelStorei(GLenum pname, GLint param)
{
    GLCALLV(PixelStorei, pname, param);
}

Q_DECL_EXPORT void APIENTRY glPixelStoref(GLenum pname, GLfloat param)
{
    GLCALLV(PixelStoref, pname, param);
}

Q_DECL_EXPORT void APIENTRY glDepthFunc(GLenum func)
{
    GLCALLV(DepthFunc, func);
}

Q_DECL_EXPORT void APIENTRY glStencilOp(GLenum fail, GLenum zfail, GLenum zpass)
{
    GLCALLV(StencilOp, fail, zfail, zpass);
}

Q_DECL_EXPORT void APIENTRY glStencilFunc(GLenum func, GLint ref, GLuint mask)
{
    GLCALLV(StencilFunc, func, ref, mask);
}

Q_DECL_EXPORT void APIENTRY glLogicOp(GLenum opcode)
{
    GLCALLV(LogicOp, opcode);
}

Q_DECL_EXPORT void APIENTRY glBlendFunc(GLenum sfactor, GLenum dfactor)
{
    GLCALLV(BlendFunc, sfactor, dfactor);
}

Q_DECL_EXPORT void APIENTRY glFlush()
{
    GLCALLV(Flush);
}

Q_DECL_EXPORT void APIENTRY glFinish()
{
    GLCALLV(Finish);
}

Q_DECL_EXPORT void APIENTRY glEnable(GLenum cap)
{
    GLCALLV(Enable, cap);
}

Q_DECL_EXPORT void APIENTRY glDisable(GLenum cap)
{
    GLCALLV(Disable, cap);
}

Q_DECL_EXPORT void APIENTRY glDepthMask(GLboolean flag)
{
    GLCALLV(DepthMask, flag);
}

Q_DECL_EXPORT void APIENTRY glColorMask(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha)
{
    GLCALLV(ColorMask, red, green, blue, alpha);
}

Q_DECL_EXPORT void APIENTRY glStencilMask(GLuint mask)
{
    GLCALLV(StencilMask, mask);
}

Q_DECL_EXPORT void APIENTRY glClearDepth(GLdouble depth)
{
    if (qgl_choose()->libraryType() == QAbstractWindowsOpenGL::DesktopGL) {
        GLCALLV(ClearDepth, depth);
    } else {
        GLCALLV(ClearDepthf, depth);
    }
}

Q_DECL_EXPORT void APIENTRY glClearStencil(GLint s)
{
    GLCALLV(ClearStencil, s);
}

Q_DECL_EXPORT void APIENTRY glClearColor(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha)
{
    GLCALLV(ClearColor, red, green, blue, alpha);
}

Q_DECL_EXPORT void APIENTRY glClear(GLbitfield mask)
{
    GLCALLV(Clear, mask);
}

Q_DECL_EXPORT void APIENTRY glDrawBuffer(GLenum mode)
{
    GLCALLV(DrawBuffer, mode);
}

Q_DECL_EXPORT void APIENTRY glTexImage2D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *pixels)
{
    GLCALLV(TexImage2D, target, level, internalformat, width, height, border, format, type, pixels);
}

Q_DECL_EXPORT void APIENTRY glTexImage1D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLint border, GLenum format, GLenum type, const GLvoid *pixels)
{
    GLCALLV(TexImage1D, target, level, internalformat, width, border, format, type, pixels);
}

Q_DECL_EXPORT void APIENTRY glTexParameteriv(GLenum target, GLenum pname, const GLint *params)
{
    GLCALLV(TexParameteriv, target, pname, params);
}

Q_DECL_EXPORT void APIENTRY glTexParameteri(GLenum target, GLenum pname, GLint param)
{
    GLCALLV(TexParameteri, target, pname, param);
}

Q_DECL_EXPORT void APIENTRY glTexParameterfv(GLenum target, GLenum pname, const GLfloat *params)
{
    GLCALLV(TexParameterfv, target, pname, params);
}

Q_DECL_EXPORT void APIENTRY glTexParameterf(GLenum target, GLenum pname, GLfloat param)
{
    GLCALLV(TexParameterf, target, pname, param);
}

Q_DECL_EXPORT void APIENTRY glScissor(GLint x, GLint y, GLsizei width, GLsizei height)
{
    GLCALLV(Scissor, x, y, width, height);
}

Q_DECL_EXPORT void APIENTRY glPolygonMode(GLenum face, GLenum mode)
{
    GLCALLV(PolygonMode, face, mode);
}

Q_DECL_EXPORT void APIENTRY glPointSize(GLfloat size)
{
    GLCALLV(PointSize, size);
}

Q_DECL_EXPORT void APIENTRY glLineWidth(GLfloat width)
{
    GLCALLV(LineWidth, width);
}

Q_DECL_EXPORT void APIENTRY glHint(GLenum target, GLenum mode)
{
    GLCALLV(Hint, target, mode);
}

Q_DECL_EXPORT void APIENTRY glFrontFace(GLenum mode)
{
    GLCALLV(FrontFace, mode);
}

Q_DECL_EXPORT void APIENTRY glCullFace(GLenum mode)
{
    GLCALLV(CullFace, mode);
}

Q_DECL_EXPORT void APIENTRY glTranslatef(GLfloat x, GLfloat y, GLfloat z)
{
    GLCALLV(Translatef, x, y, z);
}

Q_DECL_EXPORT void APIENTRY glTranslated(GLdouble x, GLdouble y, GLdouble z)
{
    GLCALLV(Translated, x, y, z);
}

Q_DECL_EXPORT void APIENTRY glScalef(GLfloat x, GLfloat y, GLfloat z)
{
    GLCALLV(Scalef, x, y, z);
}

Q_DECL_EXPORT void APIENTRY glScaled(GLdouble x, GLdouble y, GLdouble z)
{
    GLCALLV(Scaled, x, y, z);
}

Q_DECL_EXPORT void APIENTRY glRotatef(GLfloat angle, GLfloat x, GLfloat y, GLfloat z)
{
    GLCALLV(Rotatef, angle, x, y, z);
}

Q_DECL_EXPORT void APIENTRY glRotated(GLdouble angle, GLdouble x, GLdouble y, GLdouble z)
{
    GLCALLV(Rotated, angle, x, y, z);
}

Q_DECL_EXPORT void APIENTRY glPushMatrix()
{
    GLCALLV(PushMatrix);
}

Q_DECL_EXPORT void APIENTRY glPopMatrix()
{
    GLCALLV(PopMatrix);
}

Q_DECL_EXPORT void APIENTRY glOrtho(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar)
{
    GLCALLV(Ortho, left, right, bottom, top, zNear, zFar);
}

Q_DECL_EXPORT void APIENTRY glMultMatrixd(const GLdouble *m)
{
    GLCALLV(MultMatrixd, m);
}

Q_DECL_EXPORT void APIENTRY glMultMatrixf(const GLfloat *m)
{
    GLCALLV(MultMatrixf, m);
}

Q_DECL_EXPORT void APIENTRY glMatrixMode(GLenum mode)
{
    GLCALLV(MatrixMode, mode);
}

Q_DECL_EXPORT void APIENTRY glLoadMatrixd(const GLdouble *m)
{
    GLCALLV(LoadMatrixd, m);
}

Q_DECL_EXPORT void APIENTRY glLoadMatrixf(const GLfloat *m)
{
    GLCALLV(LoadMatrixf, m);
}

Q_DECL_EXPORT void APIENTRY glLoadIdentity()
{
    GLCALLV(LoadIdentity);
}

Q_DECL_EXPORT void APIENTRY glFrustum(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar)
{
    GLCALLV(Frustum, left, right, bottom, top, zNear, zFar);
}

Q_DECL_EXPORT GLboolean APIENTRY glIsList(GLuint list)
{
    GLCALL(IsList, list);
}

Q_DECL_EXPORT void APIENTRY glGetTexGeniv(GLenum coord, GLenum pname, GLint *params)
{
    GLCALLV(GetTexGeniv, coord, pname, params);
}

Q_DECL_EXPORT void APIENTRY glGetTexGenfv(GLenum coord, GLenum pname, GLfloat *params)
{
    GLCALLV(GetTexGenfv, coord, pname, params);
}

Q_DECL_EXPORT void APIENTRY glGetTexGendv(GLenum coord, GLenum pname, GLdouble *params)
{
    GLCALLV(GetTexGendv, coord, pname, params);
}

Q_DECL_EXPORT void APIENTRY glGetTexEnviv(GLenum target, GLenum pname, GLint *params)
{
    GLCALLV(GetTexEnviv, target, pname, params);
}

Q_DECL_EXPORT void APIENTRY glGetTexEnvfv(GLenum target, GLenum pname, GLfloat *params)
{
    GLCALLV(GetTexEnvfv, target, pname, params);
}

Q_DECL_EXPORT void APIENTRY glGetPolygonStipple(GLubyte *mask)
{
    GLCALLV(GetPolygonStipple, mask);
}

Q_DECL_EXPORT void APIENTRY glGetPixelMapusv(GLenum map, GLushort *values)
{
    GLCALLV(GetPixelMapusv, map, values);
}

Q_DECL_EXPORT void APIENTRY glGetPixelMapuiv(GLenum map, GLuint *values)
{
    GLCALLV(GetPixelMapuiv, map, values);
}

Q_DECL_EXPORT void APIENTRY glGetPixelMapfv(GLenum map, GLfloat *values)
{
    GLCALLV(GetPixelMapfv, map, values);
}

Q_DECL_EXPORT void APIENTRY glGetMaterialiv(GLenum face, GLenum pname, GLint *params)
{
    GLCALLV(GetMaterialiv, face, pname, params);
}

Q_DECL_EXPORT void APIENTRY glGetMaterialfv(GLenum face, GLenum pname, GLfloat *params)
{
    GLCALLV(GetMaterialfv, face, pname, params);
}

Q_DECL_EXPORT void APIENTRY glGetMapiv(GLenum target, GLenum query, GLint *v)
{
    GLCALLV(GetMapiv, target, query, v);
}

Q_DECL_EXPORT void APIENTRY glGetMapfv(GLenum target, GLenum query, GLfloat *v)
{
    GLCALLV(GetMapfv, target, query, v);
}

Q_DECL_EXPORT void APIENTRY glGetMapdv(GLenum target, GLenum query, GLdouble *v)
{
    GLCALLV(GetMapdv, target, query, v);
}

Q_DECL_EXPORT void APIENTRY glGetLightiv(GLenum light, GLenum pname, GLint *params)
{
    GLCALLV(GetLightiv, light, pname, params);
}

Q_DECL_EXPORT void APIENTRY glGetLightfv(GLenum light, GLenum pname, GLfloat *params)
{
    GLCALLV(GetLightfv, light, pname, params);
}

Q_DECL_EXPORT void APIENTRY glGetClipPlane(GLenum plane, GLdouble *equation)
{
    GLCALLV(GetClipPlane, plane, equation);
}

Q_DECL_EXPORT void APIENTRY glDrawPixels(GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels)
{
    GLCALLV(DrawPixels, width, height, format, type, pixels);
}

Q_DECL_EXPORT void APIENTRY glCopyPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum type)
{
    GLCALLV(CopyPixels, x, y, width, height, type);
}

Q_DECL_EXPORT void APIENTRY glPixelMapusv(GLenum map, GLint mapsize, const GLushort *values)
{
    GLCALLV(PixelMapusv, map, mapsize, values);
}

Q_DECL_EXPORT void APIENTRY glPixelMapuiv(GLenum map, GLint mapsize, const GLuint *values)
{
    GLCALLV(PixelMapuiv, map, mapsize, values);
}

Q_DECL_EXPORT void APIENTRY glPixelMapfv(GLenum map, GLint mapsize, const GLfloat *values)
{
    GLCALLV(PixelMapfv, map, mapsize, values);
}

Q_DECL_EXPORT void APIENTRY glPixelTransferi(GLenum pname, GLint param)
{
    GLCALLV(PixelTransferi, pname, param);
}

Q_DECL_EXPORT void APIENTRY glPixelTransferf(GLenum pname, GLfloat param)
{
    GLCALLV(PixelTransferf, pname, param);
}

Q_DECL_EXPORT void APIENTRY glPixelZoom(GLfloat xfactor, GLfloat yfactor)
{
    GLCALLV(PixelZoom, xfactor, yfactor);
}

Q_DECL_EXPORT void APIENTRY glAlphaFunc(GLenum func, GLfloat ref)
{
    GLCALLV(AlphaFunc, func, ref);
}

Q_DECL_EXPORT void APIENTRY glEvalPoint2(GLint i, GLint j)
{
    GLCALLV(EvalPoint2, i, j);
}

Q_DECL_EXPORT void APIENTRY glEvalMesh2(GLenum mode, GLint i1, GLint i2, GLint j1, GLint j2)
{
    GLCALLV(EvalMesh2, mode, i1, i2, j1, j2);
}

Q_DECL_EXPORT void APIENTRY glEvalPoint1(GLint i)
{
    GLCALLV(EvalPoint1, i);
}

Q_DECL_EXPORT void APIENTRY glEvalMesh1(GLenum mode, GLint i1, GLint i2)
{
    GLCALLV(EvalMesh1, mode, i1, i2);
}

Q_DECL_EXPORT void APIENTRY glEvalCoord2fv(const GLfloat *u)
{
    GLCALLV(EvalCoord2fv, u);
}

Q_DECL_EXPORT void APIENTRY glEvalCoord2f(GLfloat u, GLfloat v)
{
    GLCALLV(EvalCoord2f, u, v);
}

Q_DECL_EXPORT void APIENTRY glEvalCoord2dv(const GLdouble *u)
{
    GLCALLV(EvalCoord2dv, u);
}

Q_DECL_EXPORT void APIENTRY glEvalCoord2d(GLdouble u, GLdouble v)
{
    GLCALLV(EvalCoord2d, u, v);
}

Q_DECL_EXPORT void APIENTRY glEvalCoord1fv(const GLfloat *u)
{
    GLCALLV(EvalCoord1fv, u);
}

Q_DECL_EXPORT void APIENTRY glEvalCoord1f(GLfloat u)
{
    GLCALLV(EvalCoord1f, u);
}

Q_DECL_EXPORT void APIENTRY glEvalCoord1dv(const GLdouble *u)
{
    GLCALLV(EvalCoord1dv, u);
}

Q_DECL_EXPORT void APIENTRY glEvalCoord1d(GLdouble u)
{
    GLCALLV(EvalCoord1d, u);
}

Q_DECL_EXPORT void APIENTRY glMapGrid2f(GLint un, GLfloat u1, GLfloat u2, GLint vn, GLfloat v1, GLfloat v2)
{
    GLCALLV(MapGrid2f, un, u1, u2, vn, v1, v2);
}

Q_DECL_EXPORT void APIENTRY glMapGrid2d(GLint un, GLdouble u1, GLdouble u2, GLint vn, GLdouble v1, GLdouble v2)
{
    GLCALLV(MapGrid2d, un, u1, u2, vn, v1, v2);
}

Q_DECL_EXPORT void APIENTRY glMapGrid1f(GLint un, GLfloat u1, GLfloat u2)
{
    GLCALLV(MapGrid1f, un, u1, u2);
}

Q_DECL_EXPORT void APIENTRY glMapGrid1d(GLint un, GLdouble u1, GLdouble u2)
{
    GLCALLV(MapGrid1d, un, u1, u2);
}

Q_DECL_EXPORT void APIENTRY glMap2f(GLenum target, GLfloat u1, GLfloat u2, GLint ustride, GLint uorder, GLfloat v1, GLfloat v2, GLint vstride, GLint vorder, const GLfloat *points)
{
    GLCALLV(Map2f, target, u1, u2, ustride, uorder, v1, v2, vstride, vorder, points);
}

Q_DECL_EXPORT void APIENTRY glMap2d(GLenum target, GLdouble u1, GLdouble u2, GLint ustride, GLint uorder, GLdouble v1, GLdouble v2, GLint vstride, GLint vorder, const GLdouble *points)
{
    GLCALLV(Map2d, target, u1, u2, ustride, uorder, v1, v2, vstride, vorder, points);
}

Q_DECL_EXPORT void APIENTRY glMap1f(GLenum target, GLfloat u1, GLfloat u2, GLint stride, GLint order, const GLfloat *points)
{
    GLCALLV(Map1f, target, u1, u2, stride, order, points);
}

Q_DECL_EXPORT void APIENTRY glMap1d(GLenum target, GLdouble u1, GLdouble u2, GLint stride, GLint order, const GLdouble *points)
{
    GLCALLV(Map1d, target, u1, u2, stride, order, points);
}

Q_DECL_EXPORT void APIENTRY glPushAttrib(GLbitfield mask)
{
    GLCALLV(PushAttrib, mask);
}

Q_DECL_EXPORT void APIENTRY glPopAttrib()
{
    GLCALLV(PopAttrib);
}

Q_DECL_EXPORT void APIENTRY glAccum(GLenum op, GLfloat value)
{
    GLCALLV(Accum, op, value);
}

Q_DECL_EXPORT void APIENTRY glIndexMask(GLuint mask)
{
    GLCALLV(IndexMask, mask);
}

Q_DECL_EXPORT void APIENTRY glClearIndex(GLfloat c)
{
    GLCALLV(ClearIndex, c);
}

Q_DECL_EXPORT void APIENTRY glClearAccum(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha)
{
    GLCALLV(ClearAccum, red, green, blue, alpha);
}

Q_DECL_EXPORT void APIENTRY glPushName(GLuint name)
{
    GLCALLV(PushName, name);
}

Q_DECL_EXPORT void APIENTRY glPopName()
{
    GLCALLV(PopName);
}

Q_DECL_EXPORT void APIENTRY glPassThrough(GLfloat token)
{
    GLCALLV(PassThrough, token);
}

Q_DECL_EXPORT void APIENTRY glLoadName(GLuint name)
{
    GLCALLV(LoadName, name);
}

Q_DECL_EXPORT void APIENTRY glInitNames()
{
    GLCALLV(InitNames);
}

Q_DECL_EXPORT GLint APIENTRY glRenderMode(GLenum mode)
{
    GLCALL(RenderMode, mode);
}

Q_DECL_EXPORT void APIENTRY glSelectBuffer(GLsizei size, GLuint *buffer)
{
    GLCALLV(SelectBuffer, size, buffer);
}

Q_DECL_EXPORT void APIENTRY glFeedbackBuffer(GLsizei size, GLenum type, GLfloat *buffer)
{
    GLCALLV(FeedbackBuffer, size, type, buffer);
}

Q_DECL_EXPORT void APIENTRY glTexGeniv(GLenum coord, GLenum pname, const GLint *params)
{
    GLCALLV(TexGeniv, coord, pname, params);
}

Q_DECL_EXPORT void APIENTRY glTexGeni(GLenum coord, GLenum pname, GLint param)
{
    GLCALLV(TexGeni, coord, pname, param);
}

Q_DECL_EXPORT void APIENTRY glTexGenfv(GLenum coord, GLenum pname, const GLfloat *params)
{
    GLCALLV(TexGenfv, coord, pname, params);
}

Q_DECL_EXPORT void APIENTRY glTexGenf(GLenum coord, GLenum pname, GLfloat param)
{
    GLCALLV(TexGenf, coord, pname, param);
}

Q_DECL_EXPORT void APIENTRY glTexGendv(GLenum coord, GLenum pname, const GLdouble *params)
{
    GLCALLV(TexGendv, coord, pname, params);
}

Q_DECL_EXPORT void APIENTRY glTexGend(GLenum coord, GLenum pname, GLdouble param)
{
    GLCALLV(TexGend, coord, pname, param);
}

Q_DECL_EXPORT void APIENTRY glTexEnviv(GLenum target, GLenum pname, const GLint *params)
{
    GLCALLV(TexEnviv, target, pname, params);
}

Q_DECL_EXPORT void APIENTRY glTexEnvi(GLenum target, GLenum pname, GLint param)
{
    GLCALLV(TexEnvi, target, pname, param);
}

Q_DECL_EXPORT void APIENTRY glTexEnvfv(GLenum target, GLenum pname, const GLfloat *params)
{
    GLCALLV(TexEnvfv, target, pname, params);
}

Q_DECL_EXPORT void APIENTRY glTexEnvf(GLenum target, GLenum pname, GLfloat param)
{
    GLCALLV(TexEnvf, target, pname, param);
}

Q_DECL_EXPORT void APIENTRY glShadeModel(GLenum mode)
{
    GLCALLV(ShadeModel, mode);
}

Q_DECL_EXPORT void APIENTRY glPolygonStipple(const GLubyte *mask)
{
    GLCALLV(PolygonStipple, mask);
}

Q_DECL_EXPORT void APIENTRY glMaterialiv(GLenum face, GLenum pname, const GLint *params)
{
    GLCALLV(Materialiv, face, pname, params);
}

Q_DECL_EXPORT void APIENTRY glMateriali(GLenum face, GLenum pname, GLint param)
{
    GLCALLV(Materiali, face, pname, param);
}

Q_DECL_EXPORT void APIENTRY glMaterialfv(GLenum face, GLenum pname, const GLfloat *params)
{
    GLCALLV(Materialfv, face, pname, params);
}

Q_DECL_EXPORT void APIENTRY glMaterialf(GLenum face, GLenum pname, GLfloat param)
{
    GLCALLV(Materialf, face, pname, param);
}

Q_DECL_EXPORT void APIENTRY glLineStipple(GLint factor, GLushort pattern)
{
    GLCALLV(LineStipple, factor, pattern);
}

Q_DECL_EXPORT void APIENTRY glLightModeliv(GLenum pname, const GLint *params)
{
    GLCALLV(LightModeliv, pname, params);
}

Q_DECL_EXPORT void APIENTRY glLightModeli(GLenum pname, GLint param)
{
    GLCALLV(LightModeli, pname, param);
}

Q_DECL_EXPORT void APIENTRY glLightModelfv(GLenum pname, const GLfloat *params)
{
    GLCALLV(LightModelfv, pname, params);
}

Q_DECL_EXPORT void APIENTRY glLightModelf(GLenum pname, GLfloat param)
{
    GLCALLV(LightModelf, pname, param);
}

Q_DECL_EXPORT void APIENTRY glLightiv(GLenum light, GLenum pname, const GLint *params)
{
    GLCALLV(Lightiv, light, pname, params);
}

Q_DECL_EXPORT void APIENTRY glLighti(GLenum light, GLenum pname, GLint param)
{
    GLCALLV(Lighti, light, pname, param);
}

Q_DECL_EXPORT void APIENTRY glLightfv(GLenum light, GLenum pname, const GLfloat *params)
{
    GLCALLV(Lightfv, light, pname, params);
}

Q_DECL_EXPORT void APIENTRY glLightf(GLenum light, GLenum pname, GLfloat param)
{
    GLCALLV(Lightf, light, pname, param);
}

Q_DECL_EXPORT void APIENTRY glFogiv(GLenum pname, const GLint *params)
{
    GLCALLV(Fogiv, pname, params);
}

Q_DECL_EXPORT void APIENTRY glFogi(GLenum pname, GLint param)
{
    GLCALLV(Fogi, pname, param);
}

Q_DECL_EXPORT void APIENTRY glFogfv(GLenum pname, const GLfloat *params)
{
    GLCALLV(Fogfv, pname, params);
}

Q_DECL_EXPORT void APIENTRY glFogf(GLenum pname, GLfloat param)
{
    GLCALLV(Fogf, pname, param);
}

Q_DECL_EXPORT void APIENTRY glColorMaterial(GLenum face, GLenum mode)
{
    GLCALLV(ColorMaterial, face, mode);
}

Q_DECL_EXPORT void APIENTRY glClipPlane(GLenum plane, const GLdouble *equation)
{
    GLCALLV(ClipPlane, plane, equation);
}

Q_DECL_EXPORT void APIENTRY glVertex4sv(const GLshort *v)
{
    GLCALLV(Vertex4sv, v);
}

Q_DECL_EXPORT void APIENTRY glVertex4s(GLshort x, GLshort y, GLshort z, GLshort w)
{
    GLCALLV(Vertex4s, x, y, z, w);
}

Q_DECL_EXPORT void APIENTRY glVertex4iv(const GLint *v)
{
    GLCALLV(Vertex4iv, v);
}

Q_DECL_EXPORT void APIENTRY glVertex4i(GLint x, GLint y, GLint z, GLint w)
{
    GLCALLV(Vertex4i, x, y, z, w);
}

Q_DECL_EXPORT void APIENTRY glVertex4fv(const GLfloat *v)
{
    GLCALLV(Vertex4fv, v);
}

Q_DECL_EXPORT void APIENTRY glVertex4f(GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
    GLCALLV(Vertex4f, x, y, z, w);
}

Q_DECL_EXPORT void APIENTRY glVertex4dv(const GLdouble *v)
{
    GLCALLV(Vertex4dv, v);
}

Q_DECL_EXPORT void APIENTRY glVertex4d(GLdouble x, GLdouble y, GLdouble z, GLdouble w)
{
    GLCALLV(Vertex4d, x, y, z, w);
}

Q_DECL_EXPORT void APIENTRY glVertex3sv(const GLshort *v)
{
    GLCALLV(Vertex3sv, v);
}

Q_DECL_EXPORT void APIENTRY glVertex3s(GLshort x, GLshort y, GLshort z)
{
    GLCALLV(Vertex3s, x, y, z);
}

Q_DECL_EXPORT void APIENTRY glVertex3iv(const GLint *v)
{
    GLCALLV(Vertex3iv, v);
}

Q_DECL_EXPORT void APIENTRY glVertex3i(GLint x, GLint y, GLint z)
{
    GLCALLV(Vertex3i, x, y, z);
}

Q_DECL_EXPORT void APIENTRY glVertex3fv(const GLfloat *v)
{
    GLCALLV(Vertex3fv, v);
}

Q_DECL_EXPORT void APIENTRY glVertex3f(GLfloat x, GLfloat y, GLfloat z)
{
    GLCALLV(Vertex3f, x, y, z);
}

Q_DECL_EXPORT void APIENTRY glVertex3dv(const GLdouble *v)
{
    GLCALLV(Vertex3dv, v);
}

Q_DECL_EXPORT void APIENTRY glVertex3d(GLdouble x, GLdouble y, GLdouble z)
{
    GLCALLV(Vertex3d, x, y, z);
}

Q_DECL_EXPORT void APIENTRY glVertex2sv(const GLshort *v)
{
    GLCALLV(Vertex2sv, v);
}

Q_DECL_EXPORT void APIENTRY glVertex2s(GLshort x, GLshort y)
{
    GLCALLV(Vertex2s, x, y);
}

Q_DECL_EXPORT void APIENTRY glVertex2iv(const GLint *v)
{
    GLCALLV(Vertex2iv, v);
}

Q_DECL_EXPORT void APIENTRY glVertex2i(GLint x, GLint y)
{
    GLCALLV(Vertex2i, x, y);
}

Q_DECL_EXPORT void APIENTRY glVertex2fv(const GLfloat *v)
{
    GLCALLV(Vertex2fv, v);
}

Q_DECL_EXPORT void APIENTRY glVertex2f(GLfloat x, GLfloat y)
{
    GLCALLV(Vertex2f, x, y);
}

Q_DECL_EXPORT void APIENTRY glVertex2dv(const GLdouble *v)
{
    GLCALLV(Vertex2dv, v);
}

Q_DECL_EXPORT void APIENTRY glVertex2d(GLdouble x, GLdouble y)
{
    GLCALLV(Vertex2d, x, y);
}

Q_DECL_EXPORT void APIENTRY glTexCoord4sv(const GLshort *v)
{
    GLCALLV(TexCoord4sv, v);
}

Q_DECL_EXPORT void APIENTRY glTexCoord4s(GLshort s, GLshort t, GLshort r, GLshort q)
{
    GLCALLV(TexCoord4s, s, t, r, q);
}

Q_DECL_EXPORT void APIENTRY glTexCoord4iv(const GLint *v)
{
    GLCALLV(TexCoord4iv, v);
}

Q_DECL_EXPORT void APIENTRY glTexCoord4i(GLint s, GLint t, GLint r, GLint q)
{
    GLCALLV(TexCoord4i, s, t, r, q);
}

Q_DECL_EXPORT void APIENTRY glTexCoord4fv(const GLfloat *v)
{
    GLCALLV(TexCoord4fv, v);
}

Q_DECL_EXPORT void APIENTRY glTexCoord4f(GLfloat s, GLfloat t, GLfloat r, GLfloat q)
{
    GLCALLV(TexCoord4f, s, t, r, q);
}

Q_DECL_EXPORT void APIENTRY glTexCoord4dv(const GLdouble *v)
{
    GLCALLV(TexCoord4dv, v);
}

Q_DECL_EXPORT void APIENTRY glTexCoord4d(GLdouble s, GLdouble t, GLdouble r, GLdouble q)
{
    GLCALLV(TexCoord4d, s, t, r, q);
}

Q_DECL_EXPORT void APIENTRY glTexCoord3sv(const GLshort *v)
{
    GLCALLV(TexCoord3sv, v);
}

Q_DECL_EXPORT void APIENTRY glTexCoord3s(GLshort s, GLshort t, GLshort r)
{
    GLCALLV(TexCoord3s, s, t, r);
}

Q_DECL_EXPORT void APIENTRY glTexCoord3iv(const GLint *v)
{
    GLCALLV(TexCoord3iv, v);
}

Q_DECL_EXPORT void APIENTRY glTexCoord3i(GLint s, GLint t, GLint r)
{
    GLCALLV(TexCoord3i, s, t, r);
}

Q_DECL_EXPORT void APIENTRY glTexCoord3fv(const GLfloat *v)
{
    GLCALLV(TexCoord3fv, v);
}

Q_DECL_EXPORT void APIENTRY glTexCoord3f(GLfloat s, GLfloat t, GLfloat r)
{
    GLCALLV(TexCoord3f, s, t, r);
}

Q_DECL_EXPORT void APIENTRY glTexCoord3dv(const GLdouble *v)
{
    GLCALLV(TexCoord3dv, v);
}

Q_DECL_EXPORT void APIENTRY glTexCoord3d(GLdouble s, GLdouble t, GLdouble r)
{
    GLCALLV(TexCoord3d, s, t, r);
}

Q_DECL_EXPORT void APIENTRY glTexCoord2sv(const GLshort *v)
{
    GLCALLV(TexCoord2sv, v);
}

Q_DECL_EXPORT void APIENTRY glTexCoord2s(GLshort s, GLshort t)
{
    GLCALLV(TexCoord2s, s, t);
}

Q_DECL_EXPORT void APIENTRY glTexCoord2iv(const GLint *v)
{
    GLCALLV(TexCoord2iv, v);
}

Q_DECL_EXPORT void APIENTRY glTexCoord2i(GLint s, GLint t)
{
    GLCALLV(TexCoord2i, s, t);
}

Q_DECL_EXPORT void APIENTRY glTexCoord2fv(const GLfloat *v)
{
    GLCALLV(TexCoord2fv, v);
}

Q_DECL_EXPORT void APIENTRY glTexCoord2f(GLfloat s, GLfloat t)
{
    GLCALLV(TexCoord2f, s, t);
}

Q_DECL_EXPORT void APIENTRY glTexCoord2dv(const GLdouble *v)
{
    GLCALLV(TexCoord2dv, v);
}

Q_DECL_EXPORT void APIENTRY glTexCoord2d(GLdouble s, GLdouble t)
{
    GLCALLV(TexCoord2d, s, t);
}

Q_DECL_EXPORT void APIENTRY glTexCoord1sv(const GLshort *v)
{
    GLCALLV(TexCoord1sv, v);
}

Q_DECL_EXPORT void APIENTRY glTexCoord1s(GLshort s)
{
    GLCALLV(TexCoord1s, s);
}

Q_DECL_EXPORT void APIENTRY glTexCoord1iv(const GLint *v)
{
    GLCALLV(TexCoord1iv, v);
}

Q_DECL_EXPORT void APIENTRY glTexCoord1i(GLint s)
{
    GLCALLV(TexCoord1i, s);
}

Q_DECL_EXPORT void APIENTRY glTexCoord1fv(const GLfloat *v)
{
    GLCALLV(TexCoord1fv, v);
}

Q_DECL_EXPORT void APIENTRY glTexCoord1f(GLfloat s)
{
    GLCALLV(TexCoord1f, s);
}

Q_DECL_EXPORT void APIENTRY glTexCoord1dv(const GLdouble *v)
{
    GLCALLV(TexCoord1dv, v);
}

Q_DECL_EXPORT void APIENTRY glTexCoord1d(GLdouble s)
{
    GLCALLV(TexCoord1d, s);
}

Q_DECL_EXPORT void APIENTRY glRectsv(const GLshort *v1, const GLshort *v2)
{
    GLCALLV(Rectsv, v1, v2);
}

Q_DECL_EXPORT void APIENTRY glRects(GLshort x1, GLshort y1, GLshort x2, GLshort y2)
{
    GLCALLV(Rects, x1, y1, x2, y2);
}

Q_DECL_EXPORT void APIENTRY glRectiv(const GLint *v1, const GLint *v2)
{
    GLCALLV(Rectiv, v1, v2);
}

Q_DECL_EXPORT void APIENTRY glRecti(GLint x1, GLint y1, GLint x2, GLint y2)
{
    GLCALLV(Recti, x1, y1, x2, y2);
}

Q_DECL_EXPORT void APIENTRY glRectfv(const GLfloat *v1, const GLfloat *v2)
{
    GLCALLV(Rectfv, v1, v2);
}

Q_DECL_EXPORT void APIENTRY glRectf(GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2)
{
    GLCALLV(Rectf, x1, y1, x2, y2);
}

Q_DECL_EXPORT void APIENTRY glRectdv(const GLdouble *v1, const GLdouble *v2)
{
    GLCALLV(Rectdv, v1, v2);
}

Q_DECL_EXPORT void APIENTRY glRectd(GLdouble x1, GLdouble y1, GLdouble x2, GLdouble y2)
{
    GLCALLV(Rectd, x1, y1, x2, y2);
}

Q_DECL_EXPORT void APIENTRY glRasterPos4sv(const GLshort *v)
{
    GLCALLV(RasterPos4sv, v);
}

Q_DECL_EXPORT void APIENTRY glRasterPos4s(GLshort x, GLshort y, GLshort z, GLshort w)
{
    GLCALLV(RasterPos4s, x, y, z, w);
}

Q_DECL_EXPORT void APIENTRY glRasterPos4iv(const GLint *v)
{
    GLCALLV(RasterPos4iv, v);
}

Q_DECL_EXPORT void APIENTRY glRasterPos4i(GLint x, GLint y, GLint z, GLint w)
{
    GLCALLV(RasterPos4i, x, y, z, w);
}

Q_DECL_EXPORT void APIENTRY glRasterPos4fv(const GLfloat *v)
{
    GLCALLV(RasterPos4fv, v);
}

Q_DECL_EXPORT void APIENTRY glRasterPos4f(GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
    GLCALLV(RasterPos4f, x, y, z, w);
}

Q_DECL_EXPORT void APIENTRY glRasterPos4dv(const GLdouble *v)
{
    GLCALLV(RasterPos4dv, v);
}

Q_DECL_EXPORT void APIENTRY glRasterPos4d(GLdouble x, GLdouble y, GLdouble z, GLdouble w)
{
    GLCALLV(RasterPos4d, x, y, z, w);
}

Q_DECL_EXPORT void APIENTRY glRasterPos3sv(const GLshort *v)
{
    GLCALLV(RasterPos3sv, v);
}

Q_DECL_EXPORT void APIENTRY glRasterPos3s(GLshort x, GLshort y, GLshort z)
{
    GLCALLV(RasterPos3s, x, y, z);
}

Q_DECL_EXPORT void APIENTRY glRasterPos3iv(const GLint *v)
{
    GLCALLV(RasterPos3iv, v);
}

Q_DECL_EXPORT void APIENTRY glRasterPos3i(GLint x, GLint y, GLint z)
{
    GLCALLV(RasterPos3i, x, y, z);
}

Q_DECL_EXPORT void APIENTRY glRasterPos3fv(const GLfloat *v)
{
    GLCALLV(RasterPos3fv, v);
}

Q_DECL_EXPORT void APIENTRY glRasterPos3f(GLfloat x, GLfloat y, GLfloat z)
{
    GLCALLV(RasterPos3f, x, y, z);
}

Q_DECL_EXPORT void APIENTRY glRasterPos3dv(const GLdouble *v)
{
    GLCALLV(RasterPos3dv, v);
}

Q_DECL_EXPORT void APIENTRY glRasterPos3d(GLdouble x, GLdouble y, GLdouble z)
{
    GLCALLV(RasterPos3d, x, y, z);
}

Q_DECL_EXPORT void APIENTRY glRasterPos2sv(const GLshort *v)
{
    GLCALLV(RasterPos2sv, v);
}

Q_DECL_EXPORT void APIENTRY glRasterPos2s(GLshort x, GLshort y)
{
    GLCALLV(RasterPos2s, x, y);
}

Q_DECL_EXPORT void APIENTRY glRasterPos2iv(const GLint *v)
{
    GLCALLV(RasterPos2iv, v);
}

Q_DECL_EXPORT void APIENTRY glRasterPos2i(GLint x, GLint y)
{
    GLCALLV(RasterPos2i, x, y);
}

Q_DECL_EXPORT void APIENTRY glRasterPos2fv(const GLfloat *v)
{
    GLCALLV(RasterPos2fv, v);
}

Q_DECL_EXPORT void APIENTRY glRasterPos2f(GLfloat x, GLfloat y)
{
    GLCALLV(RasterPos2f, x, y);
}

Q_DECL_EXPORT void APIENTRY glRasterPos2dv(const GLdouble *v)
{
    GLCALLV(RasterPos2dv, v);
}

Q_DECL_EXPORT void APIENTRY glRasterPos2d(GLdouble x, GLdouble y)
{
    GLCALLV(RasterPos2d, x, y);
}

Q_DECL_EXPORT void APIENTRY glNormal3sv(const GLshort *v)
{
    GLCALLV(Normal3sv, v);
}

Q_DECL_EXPORT void APIENTRY glNormal3s(GLshort nx, GLshort ny, GLshort nz)
{
    GLCALLV(Normal3s, nx, ny, nz);
}

Q_DECL_EXPORT void APIENTRY glNormal3iv(const GLint *v)
{
    GLCALLV(Normal3iv, v);
}

Q_DECL_EXPORT void APIENTRY glNormal3i(GLint nx, GLint ny, GLint nz)
{
    GLCALLV(Normal3i, nx, ny, nz);
}

Q_DECL_EXPORT void APIENTRY glNormal3fv(const GLfloat *v)
{
    GLCALLV(Normal3fv, v);
}

Q_DECL_EXPORT void APIENTRY glNormal3f(GLfloat nx, GLfloat ny, GLfloat nz)
{
    GLCALLV(Normal3f, nx, ny, nz);
}

Q_DECL_EXPORT void APIENTRY glNormal3dv(const GLdouble *v)
{
    GLCALLV(Normal3dv, v);
}

Q_DECL_EXPORT void APIENTRY glNormal3d(GLdouble nx, GLdouble ny, GLdouble nz)
{
    GLCALLV(Normal3d, nx, ny, nz);
}

Q_DECL_EXPORT void APIENTRY glNormal3bv(const GLbyte *v)
{
    GLCALLV(Normal3bv, v);
}

Q_DECL_EXPORT void APIENTRY glNormal3b(GLbyte nx, GLbyte ny, GLbyte nz)
{
    GLCALLV(Normal3b, nx, ny, nz);
}

Q_DECL_EXPORT void APIENTRY glIndexsv(const GLshort *c)
{
    GLCALLV(Indexsv, c);
}

Q_DECL_EXPORT void APIENTRY glIndexs(GLshort c)
{
    GLCALLV(Indexs, c);
}

Q_DECL_EXPORT void APIENTRY glIndexiv(const GLint *c)
{
    GLCALLV(Indexiv, c);
}

Q_DECL_EXPORT void APIENTRY glIndexi(GLint c)
{
    GLCALLV(Indexi, c);
}

Q_DECL_EXPORT void APIENTRY glIndexfv(const GLfloat *c)
{
    GLCALLV(Indexfv, c);
}

Q_DECL_EXPORT void APIENTRY glIndexf(GLfloat c)
{
    GLCALLV(Indexf, c);
}

Q_DECL_EXPORT void APIENTRY glIndexdv(const GLdouble *c)
{
    GLCALLV(Indexdv, c);
}

Q_DECL_EXPORT void APIENTRY glIndexd(GLdouble c)
{
    GLCALLV(Indexd, c);
}

Q_DECL_EXPORT void APIENTRY glEnd()
{
    GLCALLV(End);
}

Q_DECL_EXPORT void APIENTRY glEdgeFlagv(const GLboolean *flag)
{
    GLCALLV(EdgeFlagv, flag);
}

Q_DECL_EXPORT void APIENTRY glEdgeFlag(GLboolean flag)
{
    GLCALLV(EdgeFlag, flag);
}

Q_DECL_EXPORT void APIENTRY glColor4usv(const GLushort *v)
{
    GLCALLV(Color4usv, v);
}

Q_DECL_EXPORT void APIENTRY glColor4us(GLushort red, GLushort green, GLushort blue, GLushort alpha)
{
    GLCALLV(Color4us, red, green, blue, alpha);
}

Q_DECL_EXPORT void APIENTRY glColor4uiv(const GLuint *v)
{
    GLCALLV(Color4uiv, v);
}

Q_DECL_EXPORT void APIENTRY glColor4ui(GLuint red, GLuint green, GLuint blue, GLuint alpha)
{
    GLCALLV(Color4ui, red, green, blue, alpha);
}

Q_DECL_EXPORT void APIENTRY glColor4ubv(const GLubyte *v)
{
    GLCALLV(Color4ubv, v);
}

Q_DECL_EXPORT void APIENTRY glColor4ub(GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha)
{
    GLCALLV(Color4ub, red, green, blue, alpha);
}

Q_DECL_EXPORT void APIENTRY glColor4sv(const GLshort *v)
{
    GLCALLV(Color4sv, v);
}

Q_DECL_EXPORT void APIENTRY glColor4s(GLshort red, GLshort green, GLshort blue, GLshort alpha)
{
    GLCALLV(Color4s, red, green, blue, alpha);
}

Q_DECL_EXPORT void APIENTRY glColor4iv(const GLint *v)
{
    GLCALLV(Color4iv, v);
}

Q_DECL_EXPORT void APIENTRY glColor4i(GLint red, GLint green, GLint blue, GLint alpha)
{
    GLCALLV(Color4i, red, green, blue, alpha);
}

Q_DECL_EXPORT void APIENTRY glColor4fv(const GLfloat *v)
{
    GLCALLV(Color4fv, v);
}

Q_DECL_EXPORT void APIENTRY glColor4f(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha)
{
    GLCALLV(Color4f, red, green, blue, alpha);
}

Q_DECL_EXPORT void APIENTRY glColor4dv(const GLdouble *v)
{
    GLCALLV(Color4dv, v);
}

Q_DECL_EXPORT void APIENTRY glColor4d(GLdouble red, GLdouble green, GLdouble blue, GLdouble alpha)
{
    GLCALLV(Color4d, red, green, blue, alpha);
}

Q_DECL_EXPORT void APIENTRY glColor4bv(const GLbyte *v)
{
    GLCALLV(Color4bv, v);
}

Q_DECL_EXPORT void APIENTRY glColor4b(GLbyte red, GLbyte green, GLbyte blue, GLbyte alpha)
{
    GLCALLV(Color4b, red, green, blue, alpha);
}

Q_DECL_EXPORT void APIENTRY glColor3usv(const GLushort *v)
{
    GLCALLV(Color3usv, v);
}

Q_DECL_EXPORT void APIENTRY glColor3us(GLushort red, GLushort green, GLushort blue)
{
    GLCALLV(Color3us, red, green, blue);
}

Q_DECL_EXPORT void APIENTRY glColor3uiv(const GLuint *v)
{
    GLCALLV(Color3uiv, v);
}

Q_DECL_EXPORT void APIENTRY glColor3ui(GLuint red, GLuint green, GLuint blue)
{
    GLCALLV(Color3ui, red, green, blue);
}

Q_DECL_EXPORT void APIENTRY glColor3ubv(const GLubyte *v)
{
    GLCALLV(Color3ubv, v);
}

Q_DECL_EXPORT void APIENTRY glColor3ub(GLubyte red, GLubyte green, GLubyte blue)
{
    GLCALLV(Color3ub, red, green, blue);
}

Q_DECL_EXPORT void APIENTRY glColor3sv(const GLshort *v)
{
    GLCALLV(Color3sv, v);
}

Q_DECL_EXPORT void APIENTRY glColor3s(GLshort red, GLshort green, GLshort blue)
{
    GLCALLV(Color3s, red, green, blue);
}

Q_DECL_EXPORT void APIENTRY glColor3iv(const GLint *v)
{
    GLCALLV(Color3iv, v);
}

Q_DECL_EXPORT void APIENTRY glColor3i(GLint red, GLint green, GLint blue)
{
    GLCALLV(Color3i, red, green, blue);
}

Q_DECL_EXPORT void APIENTRY glColor3fv(const GLfloat *v)
{
    GLCALLV(Color3fv, v);
}

Q_DECL_EXPORT void APIENTRY glColor3f(GLfloat red, GLfloat green, GLfloat blue)
{
    GLCALLV(Color3f, red, green, blue);
}

Q_DECL_EXPORT void APIENTRY glColor3dv(const GLdouble *v)
{
    GLCALLV(Color3dv, v);
}

Q_DECL_EXPORT void APIENTRY glColor3d(GLdouble red, GLdouble green, GLdouble blue)
{
    GLCALLV(Color3d, red, green, blue);
}

Q_DECL_EXPORT void APIENTRY glColor3bv(const GLbyte *v)
{
    GLCALLV(Color3bv, v);
}

Q_DECL_EXPORT void APIENTRY glColor3b(GLbyte red, GLbyte green, GLbyte blue)
{
    GLCALLV(Color3b, red, green, blue);
}

Q_DECL_EXPORT void APIENTRY glBitmap(GLsizei width, GLsizei height, GLfloat xorig, GLfloat yorig, GLfloat xmove, GLfloat ymove, const GLubyte *bitmap)
{
    GLCALLV(Bitmap, width, height, xorig, yorig, xmove, ymove, bitmap);
}

Q_DECL_EXPORT void APIENTRY glBegin(GLenum mode)
{
    GLCALLV(Begin, mode);
}

Q_DECL_EXPORT void APIENTRY glListBase(GLuint base)
{
    GLCALLV(ListBase, base);
}

Q_DECL_EXPORT GLuint APIENTRY glGenLists(GLsizei range)
{
    GLCALL(GenLists, range);
}

Q_DECL_EXPORT void APIENTRY glDeleteLists(GLuint list, GLsizei range)
{
    GLCALLV(DeleteLists, list, range);
}

Q_DECL_EXPORT void APIENTRY glCallLists(GLsizei n, GLenum type, const GLvoid *lists)
{
    GLCALLV(CallLists, n, type, lists);
}

Q_DECL_EXPORT void APIENTRY glCallList(GLuint list)
{
    GLCALLV(CallList, list);
}

Q_DECL_EXPORT void APIENTRY glEndList()
{
    GLCALLV(EndList);
}

Q_DECL_EXPORT void APIENTRY glNewList(GLuint list, GLenum mode)
{
    GLCALLV(NewList, list, mode);
}

Q_DECL_EXPORT void APIENTRY glIndexubv(const GLubyte *c)
{
    GLCALLV(Indexubv, c);
}

Q_DECL_EXPORT void APIENTRY glIndexub(GLubyte c)
{
    GLCALLV(Indexub, c);
}

Q_DECL_EXPORT GLboolean APIENTRY glIsTexture(GLuint texture)
{
    GLCALL(IsTexture, texture);
}

Q_DECL_EXPORT void APIENTRY glGenTextures(GLsizei n, GLuint *textures)
{
    GLCALLV(GenTextures, n, textures);
}

Q_DECL_EXPORT void APIENTRY glDeleteTextures(GLsizei n, const GLuint *textures)
{
    GLCALLV(DeleteTextures, n, textures);
}

Q_DECL_EXPORT void APIENTRY glBindTexture(GLenum target, GLuint texture)
{
    GLCALLV(BindTexture, target, texture);
}

Q_DECL_EXPORT void APIENTRY glTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels)
{
    GLCALLV(TexSubImage2D, target, level, xoffset, yoffset, width, height, format, type, pixels);
}

Q_DECL_EXPORT void APIENTRY glTexSubImage1D(GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const GLvoid *pixels)
{
    GLCALLV(TexSubImage1D, target, level, xoffset, width, format, type, pixels);
}

Q_DECL_EXPORT void APIENTRY glCopyTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height)
{
    GLCALLV(CopyTexSubImage2D, target, level, xoffset, yoffset, x, y, width, height);
}

Q_DECL_EXPORT void APIENTRY glCopyTexSubImage1D(GLenum target, GLint level, GLint xoffset, GLint x, GLint y, GLsizei width)
{
    GLCALLV(CopyTexSubImage1D, target, level, xoffset, x, y, width);
}

Q_DECL_EXPORT void APIENTRY glCopyTexImage2D(GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border)
{
    GLCALLV(CopyTexImage2D, target, level, internalformat, x, y, width, height, border);
}

Q_DECL_EXPORT void APIENTRY glCopyTexImage1D(GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLint border)
{
    GLCALLV(CopyTexImage1D, target, level, internalformat, x, y, width, border);
}

Q_DECL_EXPORT void APIENTRY glPolygonOffset(GLfloat factor, GLfloat units)
{
    GLCALLV(PolygonOffset, factor, units);
}

Q_DECL_EXPORT void APIENTRY glGetPointerv(GLenum pname, GLvoid* *params)
{
    GLCALLV(GetPointerv, pname, params);
}

Q_DECL_EXPORT void APIENTRY glDrawElements(GLenum mode, GLsizei count, GLenum type, const GLvoid *indices)
{
    GLCALLV(DrawElements, mode, count, type, indices);
}

Q_DECL_EXPORT void APIENTRY glDrawArrays(GLenum mode, GLint first, GLsizei count)
{
    GLCALLV(DrawArrays, mode, first, count);
}

Q_DECL_EXPORT void APIENTRY glPushClientAttrib(GLbitfield mask)
{
    GLCALLV(PushClientAttrib, mask);
}

Q_DECL_EXPORT void APIENTRY glPopClientAttrib()
{
    GLCALLV(PopClientAttrib);
}

Q_DECL_EXPORT void APIENTRY glPrioritizeTextures(GLsizei n, const GLuint *textures, const GLfloat *priorities)
{
    GLCALLV(PrioritizeTextures, n, textures, priorities);
}

Q_DECL_EXPORT GLboolean APIENTRY glAreTexturesResident(GLsizei n, const GLuint *textures, GLboolean *residences)
{
    GLCALL(AreTexturesResident, n, textures, residences);
}

Q_DECL_EXPORT void APIENTRY glVertexPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer)
{
    GLCALLV(VertexPointer, size, type, stride, pointer);
}

Q_DECL_EXPORT void APIENTRY glTexCoordPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer)
{
    GLCALLV(TexCoordPointer, size, type, stride, pointer);
}

Q_DECL_EXPORT void APIENTRY glNormalPointer(GLenum type, GLsizei stride, const GLvoid *pointer)
{
    GLCALLV(NormalPointer, type, stride, pointer);
}

Q_DECL_EXPORT void APIENTRY glInterleavedArrays(GLenum format, GLsizei stride, const GLvoid *pointer)
{
    GLCALLV(InterleavedArrays, format, stride, pointer);
}

Q_DECL_EXPORT void APIENTRY glIndexPointer(GLenum type, GLsizei stride, const GLvoid *pointer)
{
    GLCALLV(IndexPointer, type, stride, pointer);
}

Q_DECL_EXPORT void APIENTRY glEnableClientState(GLenum array)
{
    GLCALLV(EnableClientState, array);
}

Q_DECL_EXPORT void APIENTRY glEdgeFlagPointer(GLsizei stride, const GLvoid *pointer)
{
    GLCALLV(EdgeFlagPointer, stride, pointer);
}

Q_DECL_EXPORT void APIENTRY glDisableClientState(GLenum array)
{
    GLCALLV(DisableClientState, array);
}

Q_DECL_EXPORT void APIENTRY glColorPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer)
{
    GLCALLV(ColorPointer, size, type, stride, pointer);
}

Q_DECL_EXPORT void APIENTRY glArrayElement(GLint i)
{
    GLCALLV(ArrayElement, i);
}

// OpenGL ES 2.0

Q_DECL_EXPORT void APIENTRY glActiveTexture(GLenum texture)
{
    GLCALLV(ActiveTexture,texture);
}

Q_DECL_EXPORT void APIENTRY glAttachShader(GLuint program, GLuint shader)
{
    GLCALLV(AttachShader,program, shader);
}

Q_DECL_EXPORT void APIENTRY glBindAttribLocation(GLuint program, GLuint index, const GLchar* name)
{
    GLCALLV(BindAttribLocation,program, index, name);
}

Q_DECL_EXPORT void APIENTRY glBindBuffer(GLenum target, GLuint buffer)
{
    GLCALLV(BindBuffer,target, buffer);
}

Q_DECL_EXPORT void APIENTRY glBindFramebuffer(GLenum target, GLuint framebuffer)
{
    GLCALLV(BindFramebuffer,target, framebuffer);
}

Q_DECL_EXPORT void APIENTRY glBindRenderbuffer(GLenum target, GLuint renderbuffer)
{
    GLCALLV(BindRenderbuffer,target, renderbuffer);
}

Q_DECL_EXPORT void APIENTRY glBlendColor(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha)
{
    GLCALLV(BlendColor,red, green, blue, alpha);
}

Q_DECL_EXPORT void APIENTRY glBlendEquation(GLenum mode)
{
    GLCALLV(BlendEquation,mode);
}

Q_DECL_EXPORT void APIENTRY glBlendEquationSeparate(GLenum modeRGB, GLenum modeAlpha)
{
    GLCALLV(BlendEquationSeparate,modeRGB, modeAlpha);
}

Q_DECL_EXPORT void APIENTRY glBlendFuncSeparate(GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha)
{
    GLCALLV(BlendFuncSeparate,srcRGB, dstRGB, srcAlpha, dstAlpha);
}

Q_DECL_EXPORT void APIENTRY glBufferData(GLenum target, GLsizeiptr size, const GLvoid* data, GLenum usage)
{
    GLCALLV(BufferData,target, size, data, usage);
}

Q_DECL_EXPORT void APIENTRY glBufferSubData(GLenum target, GLintptr offset, GLsizeiptr size, const GLvoid* data)
{
    GLCALLV(BufferSubData,target, offset, size, data);
}

Q_DECL_EXPORT GLenum APIENTRY glCheckFramebufferStatus(GLenum target)
{
    GLCALL(CheckFramebufferStatus,target);
}

Q_DECL_EXPORT void APIENTRY glClearDepthf(GLclampf depth)
{
    glClearDepth(depth);
}

Q_DECL_EXPORT void APIENTRY glCompileShader(GLuint shader)
{
    GLCALLV(CompileShader,shader);
}

Q_DECL_EXPORT void APIENTRY glCompressedTexImage2D(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const GLvoid* data)
{
    GLCALLV(CompressedTexImage2D,target, level, internalformat, width, height, border, imageSize, data);
}

Q_DECL_EXPORT void APIENTRY glCompressedTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const GLvoid* data)
{
    GLCALLV(CompressedTexSubImage2D,target, level, xoffset, yoffset, width, height, format, imageSize, data);
}

Q_DECL_EXPORT GLuint APIENTRY glCreateProgram(void)
{
    GLCALL(CreateProgram);
}

Q_DECL_EXPORT GLuint glCreateShader(GLenum type)
{
    GLCALL(CreateShader,type);
}

Q_DECL_EXPORT void APIENTRY glDeleteBuffers(GLsizei n, const GLuint* buffers)
{
    GLCALLV(DeleteBuffers,n, buffers);
}

Q_DECL_EXPORT void APIENTRY glDeleteFramebuffers(GLsizei n, const GLuint* framebuffers)
{
    GLCALLV(DeleteFramebuffers,n, framebuffers);
}

Q_DECL_EXPORT void APIENTRY glDeleteProgram(GLuint program)
{
    GLCALLV(DeleteProgram,program);
}

Q_DECL_EXPORT void APIENTRY glDeleteRenderbuffers(GLsizei n, const GLuint* renderbuffers)
{
    GLCALLV(DeleteRenderbuffers,n, renderbuffers);
}

Q_DECL_EXPORT void APIENTRY glDeleteShader(GLuint shader)
{
    GLCALLV(DeleteShader,shader);
}

Q_DECL_EXPORT void APIENTRY glDepthRangef(GLclampf zNear, GLclampf zFar)
{
    glDepthRange(zNear, zFar);
}

Q_DECL_EXPORT void APIENTRY glDetachShader(GLuint program, GLuint shader)
{
    GLCALLV(DetachShader,program, shader);
}

Q_DECL_EXPORT void APIENTRY glDisableVertexAttribArray(GLuint index)
{
    GLCALLV(DisableVertexAttribArray,index);
}

Q_DECL_EXPORT void APIENTRY glEnableVertexAttribArray(GLuint index)
{
    GLCALLV(EnableVertexAttribArray,index);
}

Q_DECL_EXPORT void APIENTRY glFramebufferRenderbuffer(GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer)
{
    GLCALLV(FramebufferRenderbuffer,target, attachment, renderbuffertarget, renderbuffer);
}

Q_DECL_EXPORT void APIENTRY glFramebufferTexture2D(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level)
{
    GLCALLV(FramebufferTexture2D,target, attachment, textarget, texture, level);
}

Q_DECL_EXPORT void APIENTRY glGenBuffers(GLsizei n, GLuint* buffers)
{
    GLCALLV(GenBuffers,n, buffers);
}

Q_DECL_EXPORT void APIENTRY glGenerateMipmap(GLenum target)
{
    GLCALLV(GenerateMipmap,target);
}

Q_DECL_EXPORT void APIENTRY glGenFramebuffers(GLsizei n, GLuint* framebuffers)
{
    GLCALLV(GenFramebuffers,n, framebuffers);
}

Q_DECL_EXPORT void APIENTRY glGenRenderbuffers(GLsizei n, GLuint* renderbuffers)
{
    GLCALLV(GenRenderbuffers,n, renderbuffers);
}

Q_DECL_EXPORT void APIENTRY glGetActiveAttrib(GLuint program, GLuint index, GLsizei bufsize, GLsizei* length, GLint* size, GLenum* type, GLchar* name)
{
    GLCALLV(GetActiveAttrib,program, index, bufsize, length, size, type, name);
}

Q_DECL_EXPORT void APIENTRY glGetActiveUniform(GLuint program, GLuint index, GLsizei bufsize, GLsizei* length, GLint* size, GLenum* type, GLchar* name)
{
    GLCALLV(GetActiveUniform,program, index, bufsize, length, size, type, name);
}

Q_DECL_EXPORT void APIENTRY glGetAttachedShaders(GLuint program, GLsizei maxcount, GLsizei* count, GLuint* shaders)
{
    GLCALLV(GetAttachedShaders,program, maxcount, count, shaders);
}

Q_DECL_EXPORT int APIENTRY glGetAttribLocation(GLuint program, const GLchar* name)
{
    GLCALL(GetAttribLocation,program, name);
}

Q_DECL_EXPORT void APIENTRY glGetBufferParameteriv(GLenum target, GLenum pname, GLint* params)
{
    GLCALLV(GetBufferParameteriv,target, pname, params);
}

Q_DECL_EXPORT void APIENTRY glGetFramebufferAttachmentParameteriv(GLenum target, GLenum attachment, GLenum pname, GLint* params)
{
    GLCALLV(GetFramebufferAttachmentParameteriv,target, attachment, pname, params);
}

Q_DECL_EXPORT void APIENTRY glGetProgramiv(GLuint program, GLenum pname, GLint* params)
{
    GLCALLV(GetProgramiv,program, pname, params);
}

Q_DECL_EXPORT void APIENTRY glGetProgramInfoLog(GLuint program, GLsizei bufsize, GLsizei* length, GLchar* infolog)
{
    GLCALLV(GetProgramInfoLog,program, bufsize, length, infolog);
}

Q_DECL_EXPORT void APIENTRY glGetRenderbufferParameteriv(GLenum target, GLenum pname, GLint* params)
{
    GLCALLV(GetRenderbufferParameteriv,target, pname, params);
}

Q_DECL_EXPORT void APIENTRY glGetShaderiv(GLuint shader, GLenum pname, GLint* params)
{
    GLCALLV(GetShaderiv,shader, pname, params);
}

Q_DECL_EXPORT void APIENTRY glGetShaderInfoLog(GLuint shader, GLsizei bufsize, GLsizei* length, GLchar* infolog)
{
    GLCALLV(GetShaderInfoLog,shader, bufsize, length, infolog);
}

Q_DECL_EXPORT void APIENTRY glGetShaderPrecisionFormat(GLenum shadertype, GLenum precisiontype, GLint* range, GLint* precision)
{
    GLCALLV(GetShaderPrecisionFormat,shadertype, precisiontype, range, precision);
}

Q_DECL_EXPORT void APIENTRY glGetShaderSource(GLuint shader, GLsizei bufsize, GLsizei* length, GLchar* source)
{
    GLCALLV(GetShaderSource,shader, bufsize, length, source);
}

Q_DECL_EXPORT void APIENTRY glGetUniformfv(GLuint program, GLint location, GLfloat* params)
{
    GLCALLV(GetUniformfv, program, location, params);
}

Q_DECL_EXPORT void APIENTRY glGetUniformiv(GLuint program, GLint location, GLint* params)
{
    GLCALLV(GetUniformiv, program, location, params);
}

Q_DECL_EXPORT int APIENTRY glGetUniformLocation(GLuint program, const GLchar* name)
{
    GLCALL(GetUniformLocation,program, name);
}

Q_DECL_EXPORT void APIENTRY glGetVertexAttribfv(GLuint index, GLenum pname, GLfloat* params)
{
    GLCALLV(GetVertexAttribfv,index, pname, params);
}

Q_DECL_EXPORT void APIENTRY glGetVertexAttribiv(GLuint index, GLenum pname, GLint* params)
{
    GLCALLV(GetVertexAttribiv,index, pname, params);
}

Q_DECL_EXPORT void APIENTRY glGetVertexAttribPointerv(GLuint index, GLenum pname, GLvoid** pointer)
{
    GLCALLV(GetVertexAttribPointerv,index, pname, pointer);
}

Q_DECL_EXPORT GLboolean APIENTRY glIsBuffer(GLuint buffer)
{
    GLCALL(IsBuffer,buffer);
}

Q_DECL_EXPORT GLboolean APIENTRY glIsFramebuffer(GLuint framebuffer)
{
    GLCALL(IsFramebuffer,framebuffer);
}

Q_DECL_EXPORT GLboolean APIENTRY glIsProgram(GLuint program)
{
    GLCALL(IsProgram,program);
}

Q_DECL_EXPORT GLboolean APIENTRY glIsRenderbuffer(GLuint renderbuffer)
{
    GLCALL(IsRenderbuffer,renderbuffer);
}

Q_DECL_EXPORT GLboolean APIENTRY glIsShader(GLuint shader)
{
    GLCALL(IsShader,shader);
}

Q_DECL_EXPORT void APIENTRY glLinkProgram(GLuint program)
{
    GLCALLV(LinkProgram,program);
}

Q_DECL_EXPORT void APIENTRY glReleaseShaderCompiler(void)
{
    GLCALLV(ReleaseShaderCompiler,);
}

Q_DECL_EXPORT void APIENTRY glRenderbufferStorage(GLenum target, GLenum internalformat, GLsizei width, GLsizei height)
{
    GLCALLV(RenderbufferStorage,target, internalformat, width, height);
}

Q_DECL_EXPORT void APIENTRY glSampleCoverage(GLclampf value, GLboolean invert)
{
    GLCALLV(SampleCoverage,value, invert);
}

Q_DECL_EXPORT void APIENTRY glShaderBinary(GLsizei n, const GLuint* shaders, GLenum binaryformat, const GLvoid* binary, GLsizei length)
{
    GLCALLV(ShaderBinary,n, shaders, binaryformat, binary, length);
}

Q_DECL_EXPORT void APIENTRY glShaderSource(GLuint shader, GLsizei count, const GLchar* *string, const GLint* length)
{
    GLCALLV(ShaderSource,shader, count, string, length);
}

Q_DECL_EXPORT void APIENTRY glStencilFuncSeparate(GLenum face, GLenum func, GLint ref, GLuint mask)
{
    GLCALLV(StencilFuncSeparate,face, func, ref, mask);
}

Q_DECL_EXPORT void APIENTRY glStencilMaskSeparate(GLenum face, GLuint mask)
{
    GLCALLV(StencilMaskSeparate,face, mask);
}

Q_DECL_EXPORT void APIENTRY glStencilOpSeparate(GLenum face, GLenum fail, GLenum zfail, GLenum zpass)
{
    GLCALLV(StencilOpSeparate,face, fail, zfail, zpass);
}

Q_DECL_EXPORT void APIENTRY glUniform1f(GLint location, GLfloat x)
{
    GLCALLV(Uniform1f,location, x);
}

Q_DECL_EXPORT void APIENTRY glUniform1fv(GLint location, GLsizei count, const GLfloat* v)
{
    GLCALLV(Uniform1fv,location, count, v);
}

Q_DECL_EXPORT void APIENTRY glUniform1i(GLint location, GLint x)
{
    GLCALLV(Uniform1i,location, x);
}

Q_DECL_EXPORT void APIENTRY glUniform1iv(GLint location, GLsizei count, const GLint* v)
{
    GLCALLV(Uniform1iv,location, count, v);
}

Q_DECL_EXPORT void APIENTRY glUniform2f(GLint location, GLfloat x, GLfloat y)
{
    GLCALLV(Uniform2f,location, x, y);
}

Q_DECL_EXPORT void APIENTRY glUniform2fv(GLint location, GLsizei count, const GLfloat* v)
{
    GLCALLV(Uniform2fv,location, count, v);
}

Q_DECL_EXPORT void APIENTRY glUniform2i(GLint location, GLint x, GLint y)
{
    GLCALLV(Uniform2i,location, x, y);
}

Q_DECL_EXPORT void APIENTRY glUniform2iv(GLint location, GLsizei count, const GLint* v)
{
    GLCALLV(Uniform2iv,location, count, v);
}

Q_DECL_EXPORT void APIENTRY glUniform3f(GLint location, GLfloat x, GLfloat y, GLfloat z)
{
    GLCALLV(Uniform3f,location, x, y, z);
}

Q_DECL_EXPORT void APIENTRY glUniform3fv(GLint location, GLsizei count, const GLfloat* v)
{
    GLCALLV(Uniform3fv,location, count, v);
}

Q_DECL_EXPORT void APIENTRY glUniform3i(GLint location, GLint x, GLint y, GLint z)
{
    GLCALLV(Uniform3i,location, x, y, z);
}

Q_DECL_EXPORT void APIENTRY glUniform3iv(GLint location, GLsizei count, const GLint* v)
{
    GLCALLV(Uniform3iv,location, count, v);
}

Q_DECL_EXPORT void APIENTRY glUniform4f(GLint location, GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
    GLCALLV(Uniform4f,location, x, y, z, w);
}

Q_DECL_EXPORT void APIENTRY glUniform4fv(GLint location, GLsizei count, const GLfloat* v)
{
    GLCALLV(Uniform4fv,location, count, v);
}

Q_DECL_EXPORT void APIENTRY glUniform4i(GLint location, GLint x, GLint y, GLint z, GLint w)
{
    GLCALLV(Uniform4i,location, x, y, z, w);
}

Q_DECL_EXPORT void APIENTRY glUniform4iv(GLint location, GLsizei count, const GLint* v)
{
    GLCALLV(Uniform4iv,location, count, v);
}

Q_DECL_EXPORT void APIENTRY glUniformMatrix2fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value)
{
    GLCALLV(UniformMatrix2fv,location, count, transpose, value);
}

Q_DECL_EXPORT void APIENTRY glUniformMatrix3fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value)
{
    GLCALLV(UniformMatrix3fv,location, count, transpose, value);
}

Q_DECL_EXPORT void APIENTRY glUniformMatrix4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value)
{
    GLCALLV(UniformMatrix4fv,location, count, transpose, value);
}

Q_DECL_EXPORT void APIENTRY glUseProgram(GLuint program)
{
    GLCALLV(UseProgram,program);
}

Q_DECL_EXPORT void APIENTRY glValidateProgram(GLuint program)
{
    GLCALLV(ValidateProgram,program);
}

Q_DECL_EXPORT void APIENTRY glVertexAttrib1f(GLuint indx, GLfloat x)
{
    GLCALLV(VertexAttrib1f,indx, x);
}

Q_DECL_EXPORT void APIENTRY glVertexAttrib1fv(GLuint indx, const GLfloat* values)
{
    GLCALLV(VertexAttrib1fv,indx, values);
}

Q_DECL_EXPORT void APIENTRY glVertexAttrib2f(GLuint indx, GLfloat x, GLfloat y)
{
    GLCALLV(VertexAttrib2f,indx, x, y);
}

Q_DECL_EXPORT void APIENTRY glVertexAttrib2fv(GLuint indx, const GLfloat* values)
{
    GLCALLV(VertexAttrib2fv,indx, values);
}

Q_DECL_EXPORT void APIENTRY glVertexAttrib3f(GLuint indx, GLfloat x, GLfloat y, GLfloat z)
{
    GLCALLV(VertexAttrib3f,indx, x, y, z);
}

Q_DECL_EXPORT void APIENTRY glVertexAttrib3fv(GLuint indx, const GLfloat* values)
{
    GLCALLV(VertexAttrib3fv,indx, values);
}

Q_DECL_EXPORT void APIENTRY glVertexAttrib4f(GLuint indx, GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
    GLCALLV(VertexAttrib4f,indx, x, y, z, w);
}

Q_DECL_EXPORT void APIENTRY glVertexAttrib4fv(GLuint indx, const GLfloat* values)
{
    GLCALLV(VertexAttrib4fv,indx, values);
}

Q_DECL_EXPORT void APIENTRY glVertexAttribPointer(GLuint indx, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid* ptr)
{
    GLCALLV(VertexAttribPointer,indx, size, type, normalized, stride, ptr);
}

// EGL

Q_DECL_EXPORT __eglMustCastToProperFunctionPointerType EGLAPIENTRY eglGetProcAddress(const char *procname)
{
    // This is a bit more complicated since the GLES2 functions (that are not in OpenGL 1)
    // must be made queriable in order to allow classes like QOpenGLFunctions to operate
    // on the same code path for desktop GL and proxied ES.
    typedef __eglMustCastToProperFunctionPointerType FuncType;
    FuncType f = 0;
    f = qgl_choose()->EGL_GetProcAddress(procname);
    if (!f) {
        static struct Tab {
            const char *name;
            FuncType func;
        } tab[] = {
            { "glActiveTexture", (FuncType) glActiveTexture },
            { "glAttachShader", (FuncType) glAttachShader },
            { "glBindAttribLocation", (FuncType) glBindAttribLocation },
            { "glBindBuffer", (FuncType) glBindBuffer },
            { "glBindFramebuffer", (FuncType) glBindFramebuffer },
            { "glBindRenderbuffer", (FuncType) glBindRenderbuffer },
            { "glBlendColor", (FuncType) glBlendColor },
            { "glBlendEquation", (FuncType) glBlendEquation },
            { "glBlendEquationSeparate", (FuncType) glBlendEquationSeparate },
            { "glBlendFuncSeparate", (FuncType) glBlendFuncSeparate },
            { "glBufferData", (FuncType) glBufferData },
            { "glBufferSubData", (FuncType) glBufferSubData },
            { "glCheckFramebufferStatus", (FuncType) glCheckFramebufferStatus },
            { "glCompileShader", (FuncType) glCompileShader },
            { "glCompressedTexImage2D", (FuncType) glCompressedTexImage2D },
            { "glCompressedTexSubImage2D", (FuncType) glCompressedTexSubImage2D },
            { "glCreateProgram", (FuncType) glCreateProgram },
            { "glCreateShader", (FuncType) glCreateShader },
            { "glDeleteBuffers", (FuncType) glDeleteBuffers },
            { "glDeleteFramebuffers", (FuncType) glDeleteFramebuffers },
            { "glDeleteProgram", (FuncType) glDeleteProgram },
            { "glDeleteRenderbuffers", (FuncType) glDeleteRenderbuffers },
            { "glDeleteShader", (FuncType) glDeleteShader },
            { "glDetachShader", (FuncType) glDetachShader },
            { "glDisableVertexAttribArray", (FuncType) glDisableVertexAttribArray },
            { "glEnableVertexAttribArray", (FuncType) glEnableVertexAttribArray },
            { "glFramebufferRenderbuffer", (FuncType) glFramebufferRenderbuffer },
            { "glFramebufferTexture2D", (FuncType) glFramebufferTexture2D },
            { "glGenBuffers", (FuncType) glGenBuffers },
            { "glGenerateMipmap", (FuncType) glGenerateMipmap },
            { "glGenFramebuffers", (FuncType) glGenFramebuffers },
            { "glGenRenderbuffers", (FuncType) glGenRenderbuffers },
            { "glGetActiveAttrib", (FuncType) glGetActiveAttrib },
            { "glGetActiveUniform", (FuncType) glGetActiveUniform },
            { "glGetAttachedShaders", (FuncType) glGetAttachedShaders },
            { "glGetAttribLocation", (FuncType) glGetAttribLocation },
            { "glGetBufferParameteriv", (FuncType) glGetBufferParameteriv },
            { "glGetFramebufferAttachmentParameteriv", (FuncType) glGetFramebufferAttachmentParameteriv },
            { "glGetProgramiv", (FuncType) glGetProgramiv },
            { "glGetProgramInfoLog", (FuncType) glGetProgramInfoLog },
            { "glGetRenderbufferParameteriv", (FuncType) glGetRenderbufferParameteriv },
            { "glGetShaderiv", (FuncType) glGetShaderiv },
            { "glGetShaderInfoLog", (FuncType) glGetShaderInfoLog },
            { "glGetShaderPrecisionFormat", (FuncType) glGetShaderPrecisionFormat },
            { "glGetShaderSource", (FuncType) glGetShaderSource },
            { "glGetUniformfv", (FuncType) glGetUniformfv },
            { "glGetUniformiv", (FuncType) glGetUniformiv },
            { "glGetUniformLocation", (FuncType) glGetUniformLocation },
            { "glGetVertexAttribfv", (FuncType) glGetVertexAttribfv },
            { "glGetVertexAttribiv", (FuncType) glGetVertexAttribiv },
            { "glGetVertexAttribPointerv", (FuncType) glGetVertexAttribPointerv },
            { "glIsBuffer", (FuncType) glIsBuffer },
            { "glIsFramebuffer", (FuncType) glIsFramebuffer },
            { "glIsProgram", (FuncType) glIsProgram },
            { "glIsRenderbuffer", (FuncType) glIsRenderbuffer },
            { "glIsShader", (FuncType) glIsShader },
            { "glLinkProgram", (FuncType) glLinkProgram },
            { "glReleaseShaderCompiler", (FuncType) glReleaseShaderCompiler },
            { "glRenderbufferStorage", (FuncType) glRenderbufferStorage },
            { "glSampleCoverage", (FuncType) glSampleCoverage },
            { "glShaderBinary", (FuncType) glShaderBinary },
            { "glShaderSource", (FuncType) glShaderSource },
            { "glStencilFuncSeparate", (FuncType) glStencilFuncSeparate },
            { "glStencilMaskSeparate", (FuncType) glStencilMaskSeparate },
            { "glStencilOpSeparate", (FuncType) glStencilOpSeparate },
            { "glUniform1f", (FuncType) glUniform1f },
            { "glUniform1fv", (FuncType) glUniform1fv },
            { "glUniform1i", (FuncType) glUniform1i },
            { "glUniform1iv", (FuncType) glUniform1iv },
            { "glUniform2f", (FuncType) glUniform2f },
            { "glUniform2fv", (FuncType) glUniform2fv },
            { "glUniform2i", (FuncType) glUniform2i },
            { "glUniform2iv", (FuncType) glUniform2iv },
            { "glUniform3f", (FuncType) glUniform3f },
            { "glUniform3fv", (FuncType) glUniform3fv },
            { "glUniform3i", (FuncType) glUniform3i },
            { "glUniform3iv", (FuncType) glUniform3iv },
            { "glUniform4f", (FuncType) glUniform4f },
            { "glUniform4fv", (FuncType) glUniform4fv },
            { "glUniform4i", (FuncType) glUniform4i },
            { "glUniform4iv", (FuncType) glUniform4iv },
            { "glUniformMatrix2fv", (FuncType) glUniformMatrix2fv },
            { "glUniformMatrix3fv", (FuncType) glUniformMatrix3fv },
            { "glUniformMatrix4fv", (FuncType) glUniformMatrix4fv },
            { "glUseProgram", (FuncType) glUseProgram },
            { "glValidateProgram", (FuncType) glValidateProgram },
            { "glVertexAttrib1f", (FuncType) glVertexAttrib1f },
            { "glVertexAttrib1fv", (FuncType) glVertexAttrib1fv },
            { "glVertexAttrib2f", (FuncType) glVertexAttrib2f },
            { "glVertexAttrib2fv", (FuncType) glVertexAttrib2fv },
            { "glVertexAttrib3f", (FuncType) glVertexAttrib3f },
            { "glVertexAttrib3fv", (FuncType) glVertexAttrib3fv },
            { "glVertexAttrib4f", (FuncType) glVertexAttrib4f },
            { "glVertexAttrib4fv", (FuncType) glVertexAttrib4fv },
            { "glVertexAttribPointer", (FuncType) glVertexAttribPointer }
        };
        for (size_t i = 0; i < sizeof(tab) / sizeof(Tab); ++i) {
            uint len = qstrlen(tab[i].name);
            if (!qstrncmp(tab[i].name, procname, len)
                && procname[len] == '\0') {
                f = tab[i].func;
                break;
            }
        }
        if (!f)
            qCDebug(qglLc, "eglGetProcAddress failed for %s", procname);
    }

    return f;
}

} // extern "C"

// For QOpenGLFunctions
int qgl_proxyLibraryType(void)
{
    return qgl_choose()->libraryType();
}

HMODULE qgl_glHandle(void)
{
    return qgl_choose()->libraryHandle();
}

QAbstractWindowsOpenGL::QAbstractWindowsOpenGL()
    :
    CopyContext(0),
    CreateContext(0),
    CreateLayerContext(0),
    DeleteContext(0),
    GetCurrentContext(0),
    GetCurrentDC(0),
    GetProcAddress(0),
    MakeCurrent(0),
    ShareLists(0),
    UseFontBitmapsW(0),
    UseFontOutlinesW(0),
    DescribeLayerPlane(0),
    SetLayerPaletteEntries(0),
    GetLayerPaletteEntries(0),
    RealizeLayerPalette(0),
    SwapLayerBuffers(0),
    SwapMultipleBuffers(0),

    EGL_GetError(0),
    EGL_GetDisplay(0),
    EGL_Initialize(0),
    EGL_Terminate(0),
    EGL_QueryString(0),
    EGL_GetConfigs(0),
    EGL_ChooseConfig(0),
    EGL_GetConfigAttrib(0),
    EGL_CreateWindowSurface(0),
    EGL_CreatePbufferSurface(0),
    EGL_CreatePixmapSurface(0),
    EGL_DestroySurface(0),
    EGL_QuerySurface(0),
    EGL_BindAPI(0),
    EGL_QueryAPI(0),
    EGL_WaitClient(0),
    EGL_ReleaseThread(0),
    EGL_CreatePbufferFromClientBuffer(0),
    EGL_SurfaceAttrib(0),
    EGL_BindTexImage(0),
    EGL_ReleaseTexImage(0),
    EGL_SwapInterval(0),
    EGL_CreateContext(0),
    EGL_DestroyContext(0),
    EGL_MakeCurrent (0),
    EGL_GetCurrentContext(0),
    EGL_GetCurrentSurface(0),
    EGL_GetCurrentDisplay(0),
    EGL_QueryContext(0),
    EGL_WaitGL(0),
    EGL_WaitNative(0),
    EGL_SwapBuffers(0),
    EGL_CopyBuffers(0),
    EGL_GetProcAddress(0),

    Viewport(0),
    DepthRange(0),
    IsEnabled(0),
    GetTexLevelParameteriv(0),
    GetTexLevelParameterfv(0),
    GetTexParameteriv(0),
    GetTexParameterfv(0),
    GetTexImage(0),
    GetString(0),
    GetIntegerv(0),
    GetFloatv(0),
    GetError(0),
    GetDoublev(0),
    GetBooleanv(0),
    ReadPixels(0),
    ReadBuffer(0),
    PixelStorei(0),
    PixelStoref(0),
    DepthFunc(0),
    StencilOp(0),
    StencilFunc(0),
    LogicOp(0),
    BlendFunc(0),
    Flush(0),
    Finish(0),
    Enable(0),
    Disable(0),
    DepthMask(0),
    ColorMask(0),
    StencilMask(0),
    ClearDepth(0),
    ClearStencil(0),
    ClearColor(0),
    Clear(0),
    DrawBuffer(0),
    TexImage2D(0),
    TexImage1D(0),
    TexParameteriv(0),
    TexParameteri(0),
    TexParameterfv(0),
    TexParameterf(0),
    Scissor(0),
    PolygonMode(0),
    PointSize(0),
    LineWidth(0),
    Hint(0),
    FrontFace(0),
    CullFace(0),

    Translatef(0),
    Translated(0),
    Scalef(0),
    Scaled(0),
    Rotatef(0),
    Rotated(0),
    PushMatrix(0),
    PopMatrix(0),
    Ortho(0),
    MultMatrixd(0),
    MultMatrixf(0),
    MatrixMode(0),
    LoadMatrixd(0),
    LoadMatrixf(0),
    LoadIdentity(0),
    Frustum(0),
    IsList(0),
    GetTexGeniv(0),
    GetTexGenfv(0),
    GetTexGendv(0),
    GetTexEnviv(0),
    GetTexEnvfv(0),
    GetPolygonStipple(0),
    GetPixelMapusv(0),
    GetPixelMapuiv(0),
    GetPixelMapfv(0),
    GetMaterialiv(0),
    GetMaterialfv(0),
    GetMapiv(0),
    GetMapfv(0),
    GetMapdv(0),
    GetLightiv(0),
    GetLightfv(0),
    GetClipPlane(0),
    DrawPixels(0),
    CopyPixels(0),
    PixelMapusv(0),
    PixelMapuiv(0),
    PixelMapfv(0),
    PixelTransferi(0),
    PixelTransferf(0),
    PixelZoom(0),
    AlphaFunc(0),
    EvalPoint2(0),
    EvalMesh2(0),
    EvalPoint1(0),
    EvalMesh1(0),
    EvalCoord2fv(0),
    EvalCoord2f(0),
    EvalCoord2dv(0),
    EvalCoord2d(0),
    EvalCoord1fv(0),
    EvalCoord1f(0),
    EvalCoord1dv(0),
    EvalCoord1d(0),
    MapGrid2f(0),
    MapGrid2d(0),
    MapGrid1f(0),
    MapGrid1d(0),
    Map2f(0),
    Map2d(0),
    Map1f(0),
    Map1d(0),
    PushAttrib(0),
    PopAttrib(0),
    Accum(0),
    IndexMask(0),
    ClearIndex(0),
    ClearAccum(0),
    PushName(0),
    PopName(0),
    PassThrough(0),
    LoadName(0),
    InitNames(0),
    RenderMode(0),
    SelectBuffer(0),
    FeedbackBuffer(0),
    TexGeniv(0),
    TexGeni(0),
    TexGenfv(0),
    TexGenf(0),
    TexGendv(0),
    TexGend(0),
    TexEnviv(0),
    TexEnvi(0),
    TexEnvfv(0),
    TexEnvf(0),
    ShadeModel(0),
    PolygonStipple(0),
    Materialiv(0),
    Materiali(0),
    Materialfv(0),
    Materialf(0),
    LineStipple(0),
    LightModeliv(0),
    LightModeli(0),
    LightModelfv(0),
    LightModelf(0),
    Lightiv(0),
    Lighti(0),
    Lightfv(0),
    Lightf(0),
    Fogiv(0),
    Fogi(0),
    Fogfv(0),
    Fogf(0),
    ColorMaterial(0),
    ClipPlane(0),
    Vertex4sv(0),
    Vertex4s(0),
    Vertex4iv(0),
    Vertex4i(0),
    Vertex4fv(0),
    Vertex4f(0),
    Vertex4dv(0),
    Vertex4d(0),
    Vertex3sv(0),
    Vertex3s(0),
    Vertex3iv(0),
    Vertex3i(0),
    Vertex3fv(0),
    Vertex3f(0),
    Vertex3dv(0),
    Vertex3d(0),
    Vertex2sv(0),
    Vertex2s(0),
    Vertex2iv(0),
    Vertex2i(0),
    Vertex2fv(0),
    Vertex2f(0),
    Vertex2dv(0),
    Vertex2d(0),
    TexCoord4sv(0),
    TexCoord4s(0),
    TexCoord4iv(0),
    TexCoord4i(0),
    TexCoord4fv(0),
    TexCoord4f(0),
    TexCoord4dv(0),
    TexCoord4d(0),
    TexCoord3sv(0),
    TexCoord3s(0),
    TexCoord3iv(0),
    TexCoord3i(0),
    TexCoord3fv(0),
    TexCoord3f(0),
    TexCoord3dv(0),
    TexCoord3d(0),
    TexCoord2sv(0),
    TexCoord2s(0),
    TexCoord2iv(0),
    TexCoord2i(0),
    TexCoord2fv(0),
    TexCoord2f(0),
    TexCoord2dv(0),
    TexCoord2d(0),
    TexCoord1sv(0),
    TexCoord1s(0),
    TexCoord1iv(0),
    TexCoord1i(0),
    TexCoord1fv(0),
    TexCoord1f(0),
    TexCoord1dv(0),
    TexCoord1d(0),
    Rectsv(0),
    Rects(0),
    Rectiv(0),
    Recti(0),
    Rectfv(0),
    Rectf(0),
    Rectdv(0),
    Rectd(0),
    RasterPos4sv(0),
    RasterPos4s(0),
    RasterPos4iv(0),
    RasterPos4i(0),
    RasterPos4fv(0),
    RasterPos4f(0),
    RasterPos4dv(0),
    RasterPos4d(0),
    RasterPos3sv(0),
    RasterPos3s(0),
    RasterPos3iv(0),
    RasterPos3i(0),
    RasterPos3fv(0),
    RasterPos3f(0),
    RasterPos3dv(0),
    RasterPos3d(0),
    RasterPos2sv(0),
    RasterPos2s(0),
    RasterPos2iv(0),
    RasterPos2i(0),
    RasterPos2fv(0),
    RasterPos2f(0),
    RasterPos2dv(0),
    RasterPos2d(0),
    Normal3sv(0),
    Normal3s(0),
    Normal3iv(0),
    Normal3i(0),
    Normal3fv(0),
    Normal3f(0),
    Normal3dv(0),
    Normal3d(0),
    Normal3bv(0),
    Normal3b(0),
    Indexsv(0),
    Indexs(0),
    Indexiv(0),
    Indexi(0),
    Indexfv(0),
    Indexf(0),
    Indexdv(0),
    Indexd(0),
    End(0),
    EdgeFlagv(0),
    EdgeFlag(0),
    Color4usv(0),
    Color4us(0),
    Color4uiv(0),
    Color4ui(0),
    Color4ubv(0),
    Color4ub(0),
    Color4sv(0),
    Color4s(0),
    Color4iv(0),
    Color4i(0),
    Color4fv(0),
    Color4f(0),
    Color4dv(0),
    Color4d(0),
    Color4bv(0),
    Color4b(0),
    Color3usv(0),
    Color3us(0),
    Color3uiv(0),
    Color3ui(0),
    Color3ubv(0),
    Color3ub(0),
    Color3sv(0),
    Color3s(0),
    Color3iv(0),
    Color3i(0),
    Color3fv(0),
    Color3f(0),
    Color3dv(0),
    Color3d(0),
    Color3bv(0),
    Color3b(0),
    Bitmap(0),
    Begin(0),
    ListBase(0),
    GenLists(0),
    DeleteLists(0),
    CallLists(0),
    CallList(0),
    EndList(0),
    NewList(0),

    Indexubv(0),
    Indexub(0),
    IsTexture(0),
    GenTextures(0),
    DeleteTextures(0),
    BindTexture(0),
    TexSubImage2D(0),
    TexSubImage1D(0),
    CopyTexSubImage2D(0),
    CopyTexSubImage1D(0),
    CopyTexImage2D(0),
    CopyTexImage1D(0),
    PolygonOffset(0),
    GetPointerv(0),
    DrawElements(0),
    DrawArrays(0),

    PushClientAttrib(0),
    PopClientAttrib(0),
    PrioritizeTextures(0),
    AreTexturesResident(0),
    VertexPointer(0),
    TexCoordPointer(0),
    NormalPointer(0),
    InterleavedArrays(0),
    IndexPointer(0),
    EnableClientState(0),
    EdgeFlagPointer(0),
    DisableClientState(0),
    ColorPointer(0),
    ArrayElement(0),

    ActiveTexture(0),
    AttachShader(0),
    BindAttribLocation(0),
    BindBuffer(0),
    BindFramebuffer(0),
    BindRenderbuffer(0),
    BlendColor(0),
    BlendEquation(0),
    BlendEquationSeparate(0),
    BlendFuncSeparate(0),
    BufferData(0),
    BufferSubData(0),
    CheckFramebufferStatus(0),
    ClearDepthf(0),
    CompileShader(0),
    CompressedTexImage2D(0),
    CompressedTexSubImage2D(0),
    CreateProgram(0),
    CreateShader(0),
    DeleteBuffers(0),
    DeleteFramebuffers(0),
    DeleteProgram(0),
    DeleteRenderbuffers(0),
    DeleteShader(0),
    DepthRangef(0),
    DetachShader(0),
    DisableVertexAttribArray(0),
    EnableVertexAttribArray(0),
    FramebufferRenderbuffer(0),
    FramebufferTexture2D(0),
    GenBuffers(0),
    GenerateMipmap(0),
    GenFramebuffers(0),
    GenRenderbuffers(0),
    GetActiveAttrib(0),
    GetActiveUniform(0),
    GetAttachedShaders(0),
    GetAttribLocation(0),
    GetBufferParameteriv(0),
    GetFramebufferAttachmentParameteriv(0),
    GetProgramiv(0),
    GetProgramInfoLog(0),
    GetRenderbufferParameteriv(0),
    GetShaderiv(0),
    GetShaderInfoLog(0),
    GetShaderPrecisionFormat(0),
    GetShaderSource(0),
    GetUniformfv(0),
    GetUniformiv(0),
    GetUniformLocation(0),
    GetVertexAttribfv(0),
    GetVertexAttribiv(0),
    GetVertexAttribPointerv(0),
    IsBuffer(0),
    IsFramebuffer(0),
    IsProgram(0),
    IsRenderbuffer(0),
    IsShader(0),
    LinkProgram(0),
    ReleaseShaderCompiler(0),
    RenderbufferStorage(0),
    SampleCoverage(0),
    ShaderBinary(0),
    ShaderSource(0),
    StencilFuncSeparate(0),
    StencilMaskSeparate(0),
    StencilOpSeparate(0),
    Uniform1f(0),
    Uniform1fv(0),
    Uniform1i(0),
    Uniform1iv(0),
    Uniform2f(0),
    Uniform2fv(0),
    Uniform2i(0),
    Uniform2iv(0),
    Uniform3f(0),
    Uniform3fv(0),
    Uniform3i(0),
    Uniform3iv(0),
    Uniform4f(0),
    Uniform4fv(0),
    Uniform4i(0),
    Uniform4iv(0),
    UniformMatrix2fv(0),
    UniformMatrix3fv(0),
    UniformMatrix4fv(0),
    UseProgram(0),
    ValidateProgram(0),
    VertexAttrib1f(0),
    VertexAttrib1fv(0),
    VertexAttrib2f(0),
    VertexAttrib2fv(0),
    VertexAttrib3f(0),
    VertexAttrib3fv(0),
    VertexAttrib4f(0),
    VertexAttrib4fv(0),
    VertexAttribPointer(0),

    m_lib(0),
    m_libraryType(DesktopGL),
    m_loaded(false)
{
}

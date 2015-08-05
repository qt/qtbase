/*
 * Copyright (C) 2014 Canonical, Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License version 3, as published by
 * the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranties of MERCHANTABILITY,
 * SATISFACTORY QUALITY, or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "glcontext.h"
#include "window.h"
#include "logging.h"
#include <QtPlatformSupport/private/qeglconvenience_p.h>

#if !defined(QT_NO_DEBUG)
static void printOpenGLESConfig() {
  static bool once = true;
  if (once) {
    const char* string = (const char*) glGetString(GL_VENDOR);
    LOG("OpenGL ES vendor: %s", string);
    string = (const char*) glGetString(GL_RENDERER);
    LOG("OpenGL ES renderer: %s", string);
    string = (const char*) glGetString(GL_VERSION);
    LOG("OpenGL ES version: %s", string);
    string = (const char*) glGetString(GL_SHADING_LANGUAGE_VERSION);
    LOG("OpenGL ES Shading Language version: %s", string);
    string = (const char*) glGetString(GL_EXTENSIONS);
    LOG("OpenGL ES extensions: %s", string);
    once = false;
  }
}
#endif

static EGLenum api_in_use()
{
#ifdef QTUBUNTU_USE_OPENGL
    return EGL_OPENGL_API;
#else
    return EGL_OPENGL_ES_API;
#endif
}

UbuntuOpenGLContext::UbuntuOpenGLContext(UbuntuScreen* screen, UbuntuOpenGLContext* share)
{
    ASSERT(screen != NULL);
    mEglDisplay = screen->eglDisplay();
    mScreen = screen;

    // Create an OpenGL ES 2 context.
    QVector<EGLint> attribs;
    attribs.append(EGL_CONTEXT_CLIENT_VERSION);
    attribs.append(2);
    attribs.append(EGL_NONE);
    ASSERT(eglBindAPI(api_in_use()) == EGL_TRUE);

    mEglContext = eglCreateContext(mEglDisplay, screen->eglConfig(), share ? share->eglContext() : EGL_NO_CONTEXT,
                                   attribs.constData());
    DASSERT(mEglContext != EGL_NO_CONTEXT);
}

UbuntuOpenGLContext::~UbuntuOpenGLContext()
{
    ASSERT(eglDestroyContext(mEglDisplay, mEglContext) == EGL_TRUE);
}

bool UbuntuOpenGLContext::makeCurrent(QPlatformSurface* surface)
{
    DASSERT(surface->surface()->surfaceType() == QSurface::OpenGLSurface);
    EGLSurface eglSurface = static_cast<UbuntuWindow*>(surface)->eglSurface();
#if defined(QT_NO_DEBUG)
    eglBindAPI(api_in_use());
    eglMakeCurrent(mEglDisplay, eglSurface, eglSurface, mEglContext);
#else
    ASSERT(eglBindAPI(api_in_use()) == EGL_TRUE);
    ASSERT(eglMakeCurrent(mEglDisplay, eglSurface, eglSurface, mEglContext) == EGL_TRUE);
    printOpenGLESConfig();
#endif
    return true;
}

void UbuntuOpenGLContext::doneCurrent()
{
#if defined(QT_NO_DEBUG)
    eglBindAPI(api_in_use());
    eglMakeCurrent(mEglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
#else
    ASSERT(eglBindAPI(api_in_use()) == EGL_TRUE);
    ASSERT(eglMakeCurrent(mEglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT) == EGL_TRUE);
#endif
}

void UbuntuOpenGLContext::swapBuffers(QPlatformSurface* surface)
{
    UbuntuWindow *ubuntuWindow = static_cast<UbuntuWindow*>(surface);

    EGLSurface eglSurface = ubuntuWindow->eglSurface();
#if defined(QT_NO_DEBUG)
    eglBindAPI(api_in_use());
    eglSwapBuffers(mEglDisplay, eglSurface);
#else
    ASSERT(eglBindAPI(api_in_use()) == EGL_TRUE);
    ASSERT(eglSwapBuffers(mEglDisplay, eglSurface) == EGL_TRUE);
#endif

    // "Technique" copied from mir, in examples/eglapp.c around line 96
    EGLint newBufferWidth = -1;
    EGLint newBufferHeight = -1;
    /*
     * Querying the surface (actually the current buffer) dimensions here is
     * the only truly safe way to be sure that the dimensions we think we
     * have are those of the buffer being rendered to. But this should be
     * improved in future; https://bugs.launchpad.net/mir/+bug/1194384
     */
    eglQuerySurface(mEglDisplay, eglSurface, EGL_WIDTH, &newBufferWidth);
    eglQuerySurface(mEglDisplay, eglSurface, EGL_HEIGHT, &newBufferHeight);

    ubuntuWindow->onBuffersSwapped_threadSafe(newBufferWidth, newBufferHeight);
}

void (*UbuntuOpenGLContext::getProcAddress(const QByteArray& procName)) ()
{
#if defined(QT_NO_DEBUG)
    eglBindAPI(api_in_use());
#else
    ASSERT(eglBindAPI(api_in_use()) == EGL_TRUE);
#endif
    return eglGetProcAddress(procName.constData());
}

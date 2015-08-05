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

#ifndef UBUNTU_OPENGL_CONTEXT_H
#define UBUNTU_OPENGL_CONTEXT_H

#include <qpa/qplatformopenglcontext.h>
#include "screen.h"

class UbuntuOpenGLContext : public QPlatformOpenGLContext
{
public:
    UbuntuOpenGLContext(UbuntuScreen* screen, UbuntuOpenGLContext* share);
    virtual ~UbuntuOpenGLContext();

    // QPlatformOpenGLContext methods.
    QSurfaceFormat format() const override { return mScreen->surfaceFormat(); }
    void swapBuffers(QPlatformSurface* surface) override;
    bool makeCurrent(QPlatformSurface* surface) override;
    void doneCurrent() override;
    bool isValid() const override { return mEglContext != EGL_NO_CONTEXT; }
    void (*getProcAddress(const QByteArray& procName)) ();

    EGLContext eglContext() const { return mEglContext; }

private:
    UbuntuScreen* mScreen;
    EGLContext mEglContext;
    EGLDisplay mEglDisplay;
};

#endif // UBUNTU_OPENGL_CONTEXT_H

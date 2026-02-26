// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qohoseglplatformcontext.h"
#include "qohosfloatingwindow.h"
#include "qohosplatformwindow.h"
#include "render/qohosegl.h"

#include <QtGui/private/qeglpbuffer_p.h>
#include <QtGui/qsurface.h>

QT_BEGIN_NAMESPACE

QOhosEGLPlatformContext::QOhosEGLPlatformContext(const QSurfaceFormat &format, QPlatformOpenGLContext *share, EGLDisplay display)
    : QEGLPlatformContext(format, share, display, nullptr)
    , m_isDuringSwappingBuffers(false)
{
}

void QOhosEGLPlatformContext::swapBuffers(QPlatformSurface *surface)
{
    m_isDuringSwappingBuffers = true;
    QEGLPlatformContext::swapBuffers(surface);
    m_isDuringSwappingBuffers = false;

    auto *ohosWindow = static_cast<QOhosFloatingWindow *>(surface);
    auto *ohosView = ohosWindow != nullptr
        ? ohosWindow->ownedViewOrNull()
        : nullptr;
    if (ohosView != nullptr)
        ohosView->handleSurfaceContentsUpdated();
}

EGLSurface QOhosEGLPlatformContext::eglSurfaceForPlatformSurface(QPlatformSurface *surface)
{
    if (surface->surface()->surfaceClass() != QSurface::Window)
        return static_cast<QEGLPbuffer *>(surface)->pbuffer();

    auto *ohosWindow = static_cast<QOhosFloatingWindow *>(surface);
    auto *ohosWindowSurface = ohosWindow->ownedSurfaceOrNull();

    return ohosWindowSurface != nullptr
        ? ohosWindowSurface->tryGetOrCreateEGLWindowSurface(
            eglDisplay(), eglConfig(), m_isDuringSwappingBuffers)
        : EGL_NO_SURFACE;
}

QT_END_NAMESPACE

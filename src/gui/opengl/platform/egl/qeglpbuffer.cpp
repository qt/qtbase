// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtGui/qoffscreensurface.h>
#include "qeglpbuffer_p.h"
#include "qeglconvenience_p.h"

QT_BEGIN_NAMESPACE

/*!
    \class QEGLPbuffer
    \brief A pbuffer-based implementation of QPlatformOffscreenSurface for EGL.
    \since 5.2
    \internal
    \ingroup qpa

    To use this implementation in the platform plugin simply
    reimplement QPlatformIntegration::createPlatformOffscreenSurface()
    and return a new instance of this class.
*/

QEGLPbuffer::QEGLPbuffer(EGLDisplay display, const QSurfaceFormat &format, QOffscreenSurface *offscreenSurface,
                         QEGLPlatformContext::Flags flags)
    : QPlatformOffscreenSurface(offscreenSurface)
    , m_format(format)
    , m_display(display)
    , m_pbuffer(EGL_NO_SURFACE)
{
    m_hasSurfaceless = !flags.testFlag(QEGLPlatformContext::NoSurfaceless)
        && q_hasEglExtension(display, "EGL_KHR_surfaceless_context");

    if (m_hasSurfaceless)
        return;

    EGLConfig config = q_configFromGLFormat(m_display, m_format, false, EGL_PBUFFER_BIT);

    if (config) {
        const EGLint attributes[] = {
            EGL_WIDTH, offscreenSurface->size().width(),
            EGL_HEIGHT, offscreenSurface->size().height(),
            EGL_LARGEST_PBUFFER, EGL_FALSE,
            EGL_NONE
        };

        m_pbuffer = eglCreatePbufferSurface(m_display, config, attributes);

        if (m_pbuffer != EGL_NO_SURFACE)
            m_format = q_glFormatFromConfig(m_display, config);
    }
}

QEGLPbuffer::~QEGLPbuffer()
{
    if (m_pbuffer != EGL_NO_SURFACE)
        eglDestroySurface(m_display, m_pbuffer);
}

bool QEGLPbuffer::isValid() const
{
    return m_pbuffer != EGL_NO_SURFACE || m_hasSurfaceless;
}

QT_END_NAMESPACE

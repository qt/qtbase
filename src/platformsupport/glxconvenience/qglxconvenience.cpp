/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

// We have to include this before the X11 headers dragged in by
// qglxconvenience_p.h.
#include <QtCore/QByteArray>
#include <QtCore/QScopedPointer>

#include <QtCore/qmetatype.h>
#include <QtCore/qtextstream.h>
#include "qglxconvenience_p.h"

#include <QtCore/QLoggingCategory>
#include <QtCore/QVector>
#include <QtCore/QVarLengthArray>

#include <GL/glxext.h>

Q_LOGGING_CATEGORY(lcGlx, "qt.glx")

enum {
    XFocusOut = FocusOut,
    XFocusIn = FocusIn,
    XKeyPress = KeyPress,
    XKeyRelease = KeyRelease,
    XNone = None,
    XRevertToParent = RevertToParent,
    XGrayScale = GrayScale,
    XCursorShape = CursorShape
};
#undef FocusOut
#undef FocusIn
#undef KeyPress
#undef KeyRelease
#undef None
#undef RevertToParent
#undef GrayScale
#undef CursorShape

#ifdef FontChange
#undef FontChange
#endif

#ifndef GLX_FRAMEBUFFER_SRGB_CAPABLE_ARB
#define GLX_FRAMEBUFFER_SRGB_CAPABLE_ARB 0x20B2
#endif

QVector<int> qglx_buildSpec(const QSurfaceFormat &format, int drawableBit, int flags)
{
    QVector<int> spec;

    spec << GLX_LEVEL
         << 0

         << GLX_RENDER_TYPE
         << GLX_RGBA_BIT

         << GLX_RED_SIZE
         << qMax(1, format.redBufferSize())

         << GLX_GREEN_SIZE
         << qMax(1, format.greenBufferSize())

         << GLX_BLUE_SIZE
         << qMax(1, format.blueBufferSize())

         << GLX_ALPHA_SIZE
         << qMax(0, format.alphaBufferSize());

    if (format.swapBehavior() != QSurfaceFormat::SingleBuffer)
        spec << GLX_DOUBLEBUFFER
             << True;

    if (format.stereo())
        spec << GLX_STEREO
             << True;

    if (format.depthBufferSize() != -1)
        spec << GLX_DEPTH_SIZE
             << format.depthBufferSize();

    if (format.stencilBufferSize() != -1)
        spec << GLX_STENCIL_SIZE
             << format.stencilBufferSize();

    if (format.samples() > 1)
        spec << GLX_SAMPLE_BUFFERS_ARB
             << 1
             << GLX_SAMPLES_ARB
             << format.samples();

    if ((flags & QGLX_SUPPORTS_SRGB) && format.colorSpace() == QSurfaceFormat::sRGBColorSpace)
        spec << GLX_FRAMEBUFFER_SRGB_CAPABLE_ARB
             << True;

    spec << GLX_DRAWABLE_TYPE
         << drawableBit

         << XNone;

    return spec;
}

namespace  {
struct QXcbSoftwareOpenGLEnforcer {
    QXcbSoftwareOpenGLEnforcer() {
        // Allow forcing LIBGL_ALWAYS_SOFTWARE for Qt 5 applications only.
        // This is most useful with drivers that only support OpenGL 1.
        // We need OpenGL 2, but the user probably doesn't want
        // LIBGL_ALWAYS_SOFTWARE in OpenGL 1 apps.

        if (!checkedForceSoftwareOpenGL) {
            // If LIBGL_ALWAYS_SOFTWARE is already set, don't mess with it.
            // We want to unset LIBGL_ALWAYS_SOFTWARE at the end so it does not
            // get inherited by other processes, of course only if it wasn't
            // already set before.
            if (!qEnvironmentVariableIsEmpty("QT_XCB_FORCE_SOFTWARE_OPENGL")
                && !qEnvironmentVariableIsSet("LIBGL_ALWAYS_SOFTWARE"))
                forceSoftwareOpenGL = true;

            checkedForceSoftwareOpenGL = true;
        }

        if (forceSoftwareOpenGL)
            qputenv("LIBGL_ALWAYS_SOFTWARE", QByteArrayLiteral("1"));
    }

    ~QXcbSoftwareOpenGLEnforcer() {
        // unset LIBGL_ALWAYS_SOFTWARE now so other processes don't inherit it
        if (forceSoftwareOpenGL)
            qunsetenv("LIBGL_ALWAYS_SOFTWARE");
    }

    static bool checkedForceSoftwareOpenGL;
    static bool forceSoftwareOpenGL;
};

bool QXcbSoftwareOpenGLEnforcer::checkedForceSoftwareOpenGL = false;
bool QXcbSoftwareOpenGLEnforcer::forceSoftwareOpenGL = false;

template <class T>
struct QXlibScopedPointerDeleter {
    static inline void cleanup(T *pointer) {
        if (pointer)
            XFree(pointer);
    }
};

template <class T>
using QXlibPointer = QScopedPointer<T, QXlibScopedPointerDeleter<T>>;

template <class T>
using QXlibArrayPointer = QScopedArrayPointer<T, QXlibScopedPointerDeleter<T>>;
}

GLXFBConfig qglx_findConfig(Display *display, int screen , QSurfaceFormat format, bool highestPixelFormat, int drawableBit, int flags)
{
    QXcbSoftwareOpenGLEnforcer softwareOpenGLEnforcer;

    GLXFBConfig config = nullptr;

    do {
        const QVector<int> spec = qglx_buildSpec(format, drawableBit, flags);

        int confcount = 0;
        QXlibArrayPointer<GLXFBConfig> configs(glXChooseFBConfig(display, screen, spec.constData(), &confcount));

        if (!config && confcount > 0) {
            config = configs[0];
            if (highestPixelFormat && !format.hasAlpha())
                break;
        }

        const int requestedRed = qMax(0, format.redBufferSize());
        const int requestedGreen = qMax(0, format.greenBufferSize());
        const int requestedBlue = qMax(0, format.blueBufferSize());
        const int requestedAlpha = qMax(0, format.alphaBufferSize());

        GLXFBConfig compatibleCandidate = nullptr;
        for (int i = 0; i < confcount; i++) {
            GLXFBConfig candidate = configs[i];

            if ((flags & QGLX_SUPPORTS_SRGB) && format.colorSpace() == QSurfaceFormat::sRGBColorSpace) {
                int srgbCapable = 0;
                glXGetFBConfigAttrib(display, candidate, GLX_FRAMEBUFFER_SRGB_CAPABLE_ARB, &srgbCapable);
                if (!srgbCapable)
                    continue;
            }

            QXlibPointer<XVisualInfo> visual(glXGetVisualFromFBConfig(display, candidate));
            if (!visual)
                continue;
            int actualRed;
            int actualGreen;
            int actualBlue;
            int actualAlpha;
            glXGetFBConfigAttrib(display, candidate, GLX_RED_SIZE, &actualRed);
            glXGetFBConfigAttrib(display, candidate, GLX_GREEN_SIZE, &actualGreen);
            glXGetFBConfigAttrib(display, candidate, GLX_BLUE_SIZE, &actualBlue);
            glXGetFBConfigAttrib(display, candidate, GLX_ALPHA_SIZE, &actualAlpha);
            // Sometimes the visuals don't have a depth that includes the alpha channel.
            actualAlpha = qMin(actualAlpha, visual->depth - actualRed - actualGreen - actualBlue);

            if (requestedRed && actualRed < requestedRed)
                continue;
            if (requestedGreen && actualGreen < requestedGreen)
                continue;
            if (requestedBlue && actualBlue < requestedBlue)
                continue;
            if (requestedAlpha && actualAlpha < requestedAlpha)
                continue;
            if (!compatibleCandidate) // Only pick up the first compatible one offered by the server
                compatibleCandidate = candidate;

            if (requestedRed && actualRed != requestedRed)
                continue;
            if (requestedGreen && actualGreen != requestedGreen)
                continue;
            if (requestedBlue && actualBlue != requestedBlue)
                continue;
            if (requestedAlpha && actualAlpha != requestedAlpha)
                continue;

            return candidate;
        }
        if (compatibleCandidate) {
            qCDebug(lcGlx) << "qglx_findConfig: Found non-matching but compatible FBConfig";
            return compatibleCandidate;
        }
    } while (qglx_reduceFormat(&format));

    if (!config)
        qCWarning(lcGlx) << "qglx_findConfig: Failed to finding matching FBConfig for" << format;

    return config;
}

XVisualInfo *qglx_findVisualInfo(Display *display, int screen, QSurfaceFormat *format, int drawableBit, int flags)
{
    Q_ASSERT(format);

    XVisualInfo *visualInfo = nullptr;

    GLXFBConfig config = qglx_findConfig(display, screen, *format, false, drawableBit, flags);
    if (config)
        visualInfo = glXGetVisualFromFBConfig(display, config);

    if (visualInfo) {
        qglx_surfaceFormatFromGLXFBConfig(format, display, config, flags);
        return visualInfo;
    }

    // attempt to fall back to glXChooseVisual
    do {
        QVector<int> attribs = qglx_buildSpec(*format, drawableBit, flags);
        visualInfo = glXChooseVisual(display, screen, attribs.data());

        if (visualInfo) {
            qglx_surfaceFormatFromVisualInfo(format, display, visualInfo, flags);
            return visualInfo;
        }
    } while (qglx_reduceFormat(format));

    return visualInfo;
}

void qglx_surfaceFormatFromGLXFBConfig(QSurfaceFormat *format, Display *display, GLXFBConfig config, int flags)
{
    int redSize     = 0;
    int greenSize   = 0;
    int blueSize    = 0;
    int alphaSize   = 0;
    int depthSize   = 0;
    int stencilSize = 0;
    int sampleBuffers = 0;
    int sampleCount = 0;
    int stereo      = 0;
    int srgbCapable = 0;

    glXGetFBConfigAttrib(display, config, GLX_RED_SIZE,     &redSize);
    glXGetFBConfigAttrib(display, config, GLX_GREEN_SIZE,   &greenSize);
    glXGetFBConfigAttrib(display, config, GLX_BLUE_SIZE,    &blueSize);
    glXGetFBConfigAttrib(display, config, GLX_ALPHA_SIZE,   &alphaSize);
    glXGetFBConfigAttrib(display, config, GLX_DEPTH_SIZE,   &depthSize);
    glXGetFBConfigAttrib(display, config, GLX_STENCIL_SIZE, &stencilSize);
    glXGetFBConfigAttrib(display, config, GLX_SAMPLE_BUFFERS_ARB, &sampleBuffers);
    glXGetFBConfigAttrib(display, config, GLX_STEREO,       &stereo);
    if (flags & QGLX_SUPPORTS_SRGB)
        glXGetFBConfigAttrib(display, config, GLX_FRAMEBUFFER_SRGB_CAPABLE_ARB, &srgbCapable);

    format->setRedBufferSize(redSize);
    format->setGreenBufferSize(greenSize);
    format->setBlueBufferSize(blueSize);
    format->setAlphaBufferSize(alphaSize);
    format->setDepthBufferSize(depthSize);
    format->setStencilBufferSize(stencilSize);
    if (sampleBuffers) {
        glXGetFBConfigAttrib(display, config, GLX_SAMPLES_ARB, &sampleCount);
        format->setSamples(sampleCount);
    }
    format->setColorSpace(srgbCapable ? QSurfaceFormat::sRGBColorSpace : QSurfaceFormat::DefaultColorSpace);

    format->setStereo(stereo);
}

void qglx_surfaceFormatFromVisualInfo(QSurfaceFormat *format, Display *display, XVisualInfo *visualInfo, int flags)
{
    int redSize     = 0;
    int greenSize   = 0;
    int blueSize    = 0;
    int alphaSize   = 0;
    int depthSize   = 0;
    int stencilSize = 0;
    int sampleBuffers = 0;
    int sampleCount = 0;
    int stereo      = 0;
    int srgbCapable = 0;

    glXGetConfig(display, visualInfo, GLX_RED_SIZE,     &redSize);
    glXGetConfig(display, visualInfo, GLX_GREEN_SIZE,   &greenSize);
    glXGetConfig(display, visualInfo, GLX_BLUE_SIZE,    &blueSize);
    glXGetConfig(display, visualInfo, GLX_ALPHA_SIZE,   &alphaSize);
    glXGetConfig(display, visualInfo, GLX_DEPTH_SIZE,   &depthSize);
    glXGetConfig(display, visualInfo, GLX_STENCIL_SIZE, &stencilSize);
    glXGetConfig(display, visualInfo, GLX_SAMPLE_BUFFERS_ARB, &sampleBuffers);
    glXGetConfig(display, visualInfo, GLX_STEREO,       &stereo);
    if (flags & QGLX_SUPPORTS_SRGB)
        glXGetConfig(display, visualInfo, GLX_FRAMEBUFFER_SRGB_CAPABLE_ARB, &srgbCapable);

    format->setRedBufferSize(redSize);
    format->setGreenBufferSize(greenSize);
    format->setBlueBufferSize(blueSize);
    format->setAlphaBufferSize(alphaSize);
    format->setDepthBufferSize(depthSize);
    format->setStencilBufferSize(stencilSize);
    if (sampleBuffers) {
        glXGetConfig(display, visualInfo, GLX_SAMPLES_ARB, &sampleCount);
        format->setSamples(sampleCount);
    }
    format->setColorSpace(srgbCapable ? QSurfaceFormat::sRGBColorSpace : QSurfaceFormat::DefaultColorSpace);

    format->setStereo(stereo);
}

bool qglx_reduceFormat(QSurfaceFormat *format)
{
    Q_ASSERT(format);
    if (std::max(std::max(format->redBufferSize(), format->greenBufferSize()), format->blueBufferSize()) > 8) {
        if (format->alphaBufferSize() > 2) {
            // First try to match 10 10 10 2
            format->setAlphaBufferSize(2);
            return true;
        }

        format->setRedBufferSize(std::min(format->redBufferSize(), 8));
        format->setGreenBufferSize(std::min(format->greenBufferSize(), 8));
        format->setBlueBufferSize(std::min(format->blueBufferSize(), 8));
        return true;
    }

    if (format->redBufferSize() > 1) {
        format->setRedBufferSize(1);
        return true;
    }

    if (format->greenBufferSize() > 1) {
        format->setGreenBufferSize(1);
        return true;
    }

    if (format->blueBufferSize() > 1) {
        format->setBlueBufferSize(1);
        return true;
    }

    if (format->swapBehavior() != QSurfaceFormat::SingleBuffer){
        format->setSwapBehavior(QSurfaceFormat::SingleBuffer);
        return true;
    }

    if (format->samples() > 1) {
        format->setSamples(qMin(16, format->samples() / 2));
        return true;
    }

    if (format->depthBufferSize() >= 32) {
        format->setDepthBufferSize(24);
        return true;
    }

    if (format->depthBufferSize() > 1) {
        format->setDepthBufferSize(1);
        return true;
    }

    if (format->depthBufferSize() > 0) {
        format->setDepthBufferSize(0);
        return true;
    }

    if (format->hasAlpha()) {
        format->setAlphaBufferSize(0);
        return true;
    }

    if (format->stencilBufferSize() > 1) {
        format->setStencilBufferSize(1);
        return true;
    }

    if (format->stencilBufferSize() > 0) {
        format->setStencilBufferSize(0);
        return true;
    }

    if (format->stereo()) {
        format->setStereo(false);
        return true;
    }

    if (format->colorSpace() == QSurfaceFormat::sRGBColorSpace) {
        format->setColorSpace(QSurfaceFormat::DefaultColorSpace);
        return true;
    }

    return false;
}

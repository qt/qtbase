/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qglxconvenience.h"

#include <QtCore/QVector>

#ifndef QT_NO_XRENDER
#include <X11/extensions/Xrender.h>
#endif

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

QVector<int> qglx_buildSpec(const QWindowFormat &format, int drawableBit)
{
    QVector<int> spec(48);
    int i = 0;

    spec[i++] = GLX_LEVEL;
    spec[i++] = 0;
    spec[i++] = GLX_DRAWABLE_TYPE; spec[i++] = drawableBit;

    spec[i++] = GLX_RENDER_TYPE; spec[i++] = GLX_RGBA_BIT;

    spec[i++] = GLX_RED_SIZE; spec[i++] = (format.redBufferSize() == -1) ? 1 : format.redBufferSize();
    spec[i++] = GLX_GREEN_SIZE; spec[i++] =  (format.greenBufferSize() == -1) ? 1 : format.greenBufferSize();
    spec[i++] = GLX_BLUE_SIZE; spec[i++] = (format.blueBufferSize() == -1) ? 1 : format.blueBufferSize();
    if (format.hasAlpha()) {
        spec[i++] = GLX_ALPHA_SIZE; spec[i++] = format.alphaBufferSize();
    }

    spec[i++] = GLX_DOUBLEBUFFER; spec[i++] = format.swapBehavior() != QWindowFormat::SingleBuffer ? True : False;

    spec[i++] = GLX_STEREO; spec[i++] =  format.stereo() ? True : False;

    if (format.depthBufferSize() > 0) {
        spec[i++] = GLX_DEPTH_SIZE; spec[i++] = format.depthBufferSize();
    }

    if (format.stencilBufferSize() > 0) {
        spec[i++] = GLX_STENCIL_SIZE; spec[i++] =  (format.stencilBufferSize() == -1) ? 1 : format.stencilBufferSize();
    }

    if (format.samples() > 1) {
        spec[i++] = GLX_SAMPLE_BUFFERS_ARB;
        spec[i++] = 1;
        spec[i++] = GLX_SAMPLES_ARB;
        spec[i++] = format.samples();
    }

    spec[i++] = XNone;
    return spec;
}

GLXFBConfig qglx_findConfig(Display *display, int screen , const QWindowFormat &format, int drawableBit)
{
    bool reduced = true;
    GLXFBConfig chosenConfig = 0;
    QWindowFormat reducedFormat = format;
    while (!chosenConfig && reduced) {
        QVector<int> spec = qglx_buildSpec(reducedFormat, drawableBit);
        int confcount = 0;
        GLXFBConfig *configs;
        configs = glXChooseFBConfig(display, screen,spec.constData(),&confcount);
        if (confcount)
        {
            for (int i = 0; i < confcount; i++) {
                chosenConfig = configs[i];
                // Make sure we try to get an ARGB visual if the format asked for an alpha:
                if (reducedFormat.hasAlpha()) {
                    int alphaSize;
                    glXGetFBConfigAttrib(display,configs[i],GLX_ALPHA_SIZE,&alphaSize);
                    if (alphaSize > 0) {
                        XVisualInfo *visual = glXGetVisualFromFBConfig(display, chosenConfig);
#if !defined(QT_NO_XRENDER)
                        XRenderPictFormat *pictFormat = XRenderFindVisualFormat(display, visual->visual);
                        if (pictFormat->direct.alphaMask > 0)
                            break;
#else
                        if (visual->depth == 32)
                            break;
#endif
                    }
                } else {
                    break; // Just choose the first in the list if there's no alpha requested
                }
            }

            XFree(configs);
        }
        reducedFormat = qglx_reduceWindowFormat(reducedFormat,&reduced);
    }

    if (!chosenConfig)
        qWarning("Warning: no suitable glx confiuration found");

    return chosenConfig;
}

XVisualInfo *qglx_findVisualInfo(Display *display, int screen, const QWindowFormat &format)
{
    GLXFBConfig config = qglx_findConfig(display,screen,format);
    XVisualInfo *visualInfo = glXGetVisualFromFBConfig(display,config);
    return visualInfo;
}

QWindowFormat qglx_platformWindowFromGLXFBConfig(Display *display, GLXFBConfig config, GLXContext)
{
    QWindowFormat format;
    int redSize     = 0;
    int greenSize   = 0;
    int blueSize    = 0;
    int alphaSize   = 0;
    int depthSize   = 0;
    int stencilSize = 0;
    int sampleBuffers = 0;
    int sampleCount = 0;
    int stereo      = 0;

    XVisualInfo *vi = glXGetVisualFromFBConfig(display,config);
    XFree(vi);
    glXGetFBConfigAttrib(display, config, GLX_RED_SIZE,     &redSize);
    glXGetFBConfigAttrib(display, config, GLX_GREEN_SIZE,   &greenSize);
    glXGetFBConfigAttrib(display, config, GLX_BLUE_SIZE,    &blueSize);
    glXGetFBConfigAttrib(display, config, GLX_ALPHA_SIZE,   &alphaSize);
    glXGetFBConfigAttrib(display, config, GLX_DEPTH_SIZE,   &depthSize);
    glXGetFBConfigAttrib(display, config, GLX_STENCIL_SIZE, &stencilSize);
    glXGetFBConfigAttrib(display, config, GLX_SAMPLES,      &sampleBuffers);
    glXGetFBConfigAttrib(display, config, GLX_STEREO,       &stereo);

    format.setRedBufferSize(redSize);
    format.setGreenBufferSize(greenSize);
    format.setBlueBufferSize(blueSize);
    format.setAlphaBufferSize(alphaSize);
    format.setDepthBufferSize(depthSize);
    format.setStencilBufferSize(stencilSize);
    if (sampleBuffers) {
        glXGetFBConfigAttrib(display, config, GLX_SAMPLES_ARB, &sampleCount);
        format.setSamples(sampleCount);
    }

    format.setStereo(stereo);

    return format;
}

QWindowFormat qglx_reduceWindowFormat(const QWindowFormat &format, bool *reduced)
{
    QWindowFormat retFormat = format;
    *reduced = true;

    if (retFormat.samples() > 1) {
        retFormat.setSamples(0);
    } else if (retFormat.stereo()) {
        retFormat.setStereo(false);
    }else if (retFormat.stencilBufferSize() > 0) {
        retFormat.setStencilBufferSize(0);
    }else if (retFormat.hasAlpha()) {
        retFormat.setAlphaBufferSize(0);
    }else if (retFormat.depthBufferSize() > 0) {
        retFormat.setDepthBufferSize(0);
    }else{
        *reduced = false;
    }
    return retFormat;
}

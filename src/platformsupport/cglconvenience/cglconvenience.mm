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

#include "cglconvenience_p.h"
#include <QtCore/qglobal.h>
#include <QtCore/private/qcore_mac_p.h>
#include <AppKit/AppKit.h>
#include <QVector>
#include <qdebug.h>

// Match up with createNSOpenGLPixelFormat below!
QSurfaceFormat qcgl_surfaceFormat()
{
    QSurfaceFormat format;
    format.setRenderableType(QSurfaceFormat::OpenGL);
    format.setRedBufferSize(8);
    format.setGreenBufferSize(8);
    format.setBlueBufferSize(8);
    format.setAlphaBufferSize(8);
/*
    format.setDepthBufferSize(24);
    format.setAccumBufferSize(0);
    format.setStencilBufferSize(8);
    format.setSampleBuffers(false);
    format.setSamples(1);
    format.setDepth(true);
    format.setRgba(true);
    format.setAlpha(true);
    format.setAccum(false);
    format.setStencil(true);
    format.setStereo(false);
    format.setDirectRendering(false);
*/
    return format;
}

void *qcgl_createNSOpenGLPixelFormat(const QSurfaceFormat &format)
{

    QVector<NSOpenGLPixelFormatAttribute> attrs;

    if (format.swapBehavior() == QSurfaceFormat::DoubleBuffer
        || format.swapBehavior() == QSurfaceFormat::DefaultSwapBehavior)
        attrs.append(NSOpenGLPFADoubleBuffer);
    else if (format.swapBehavior() == QSurfaceFormat::TripleBuffer)
        attrs.append(NSOpenGLPFATripleBuffer);

    if (format.profile() == QSurfaceFormat::CoreProfile
            && ((format.majorVersion() == 3 && format.minorVersion() >= 2)
                || format.majorVersion() > 3)) {
        attrs << NSOpenGLPFAOpenGLProfile;
        attrs << NSOpenGLProfileVersion3_2Core;
    } else {
        attrs << NSOpenGLPFAOpenGLProfile;
        attrs << NSOpenGLProfileVersionLegacy;
    }

    if (format.depthBufferSize() > 0)
        attrs <<  NSOpenGLPFADepthSize << format.depthBufferSize();
    if (format.stencilBufferSize() > 0)
        attrs << NSOpenGLPFAStencilSize << format.stencilBufferSize();
    if (format.alphaBufferSize() > 0)
        attrs << NSOpenGLPFAAlphaSize << format.alphaBufferSize();
    if ((format.redBufferSize() > 0) &&
        (format.greenBufferSize() > 0) &&
        (format.blueBufferSize() > 0)) {
        const int colorSize = format.redBufferSize() +
                              format.greenBufferSize() +
                              format.blueBufferSize();
        attrs << NSOpenGLPFAColorSize << colorSize << NSOpenGLPFAMinimumPolicy;
    }

    if (format.samples() > 0) {
        attrs << NSOpenGLPFAMultisample
              << NSOpenGLPFASampleBuffers << (NSOpenGLPixelFormatAttribute) 1
              << NSOpenGLPFASamples << (NSOpenGLPixelFormatAttribute) format.samples();
    }

    if (format.stereo())
        attrs << NSOpenGLPFAStereo;

    attrs << NSOpenGLPFAAllowOfflineRenderers;

    QByteArray useLayer = qgetenv("QT_MAC_WANTS_LAYER");
    if (!useLayer.isEmpty() && useLayer.toInt() > 0) {
        // Disable the software rendering fallback. This makes compositing
        // OpenGL and raster NSViews using Core Animation layers possible.
        attrs << NSOpenGLPFANoRecovery;
    }

    attrs << 0;

    NSOpenGLPixelFormat* pixelFormat = [[NSOpenGLPixelFormat alloc] initWithAttributes:attrs.constData()];
    return pixelFormat;
}

/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "cglconvenience_p.h"
#include <QtCore/qglobal.h>
#include <QtCore/private/qcore_mac_p.h>
#include <Cocoa/Cocoa.h>
#include <QVector>

void (*qcgl_getProcAddress(const QByteArray &procName))()
{
    CFURLRef url = CFURLCreateWithFileSystemPath(kCFAllocatorDefault,
            CFSTR("/System/Library/Frameworks/OpenGL.framework"), kCFURLPOSIXPathStyle, false);
    CFBundleRef bundle = CFBundleCreate(kCFAllocatorDefault, url);
    CFStringRef procNameCF = QCFString::toCFStringRef(QString::fromLatin1(procName.constData()));
    void *proc = CFBundleGetFunctionPointerForName(bundle, procNameCF);
    CFRelease(url);
    CFRelease(bundle);
    CFRelease(procNameCF);
    return (void (*) ())proc;
}

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

    if (format.swapBehavior() != QSurfaceFormat::SingleBuffer)
        attrs.append(NSOpenGLPFADoubleBuffer);

#if MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_7
    if (QSysInfo::MacintoshVersion >= QSysInfo::MV_10_7) {
        if (format.profile() == QSurfaceFormat::CoreProfile
                && ((format.majorVersion() == 3 && format.minorVersion() >= 2)
                    || format.majorVersion() > 3)) {
            attrs << NSOpenGLPFAOpenGLProfile;
            attrs << NSOpenGLProfileVersion3_2Core;
        } else {
            attrs << NSOpenGLPFAOpenGLProfile;
            attrs << NSOpenGLProfileVersionLegacy;
        }
    }
#else
    if (format.profile() == QSurfaceFormat::CoreProfile)
        qWarning("Mac OSX >= 10.7 is needed for OpenGL Core Profile support");
#endif

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

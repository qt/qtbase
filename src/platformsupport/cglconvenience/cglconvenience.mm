/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: http://www.qt-project.org/
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

#include "cglconvenience_p.h"
#include <QtCore/private/qcore_mac_p.h>
#include <Cocoa/Cocoa.h>
#include <QVector>

void (*qcgl_getProcAddress(const QByteArray &procName))()
{
    CFURLRef url = CFURLCreateWithFileSystemPath(kCFAllocatorDefault,
            CFSTR("/System/Library/Frameworks/OpenGL.framework"), kCFURLPOSIXPathStyle, false);
    CFBundleRef bundle = CFBundleCreate(kCFAllocatorDefault, url);
    CFStringRef procNameCF = QCFString::toCFStringRef(QString::fromAscii(procName.constData()));
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

    attrs.append(NSOpenGLPFADoubleBuffer);

    if (format.depthBufferSize() > 0)
        attrs <<  NSOpenGLPFADepthSize << format.depthBufferSize();
    if (format.stencilBufferSize() > 0)
        attrs << NSOpenGLPFAStencilSize << format.stencilBufferSize();
    if (format.alphaBufferSize() > 0)
        attrs << NSOpenGLPFAAlphaSize << format.alphaBufferSize();

    if (format.samples() > 0) {
        attrs << NSOpenGLPFAMultisample
              << NSOpenGLPFASampleBuffers << (NSOpenGLPixelFormatAttribute) 1
              << NSOpenGLPFASamples << (NSOpenGLPixelFormatAttribute) format.samples();
    }

    attrs << 0;

    NSOpenGLPixelFormat* pixelFormat = [[NSOpenGLPixelFormat alloc] initWithAttributes:attrs.constData()];
    return pixelFormat;
}

CGLContextObj qcgl_createGlContext()
{
    CGLContextObj context;
    NSOpenGLPixelFormat *format = reinterpret_cast<NSOpenGLPixelFormat *>(qcgl_createNSOpenGLPixelFormat(qcgl_surfaceFormat()));
    CGLPixelFormatObj cglFormat = static_cast<CGLPixelFormatObj>([format CGLPixelFormatObj]);
    CGLCreateContext(cglFormat ,NULL, &context);
    return context;
}


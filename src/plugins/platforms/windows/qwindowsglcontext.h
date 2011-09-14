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

#ifndef QWINDOWSGLCONTEXT_H
#define QWINDOWSGLCONTEXT_H

#include "array.h"
#include "qtwindows_additional.h"

#include <QtGui/QPlatformOpenGLContext>
#include <QtGui/QOpenGLContext>
#include <QtCore/QSharedPointer>

QT_BEGIN_NAMESPACE

class QDebug;

enum QWindowsGLFormatFlags
{
    QWindowsGLDirectRendering = 0x1,
    QWindowsGLOverlay = 0x2,
    QWindowsGLRenderToPixmap = 0x4,
    QWindowsGLAccumBuffer = 0x8,
    QWindowsGLDeprecatedFunctions = 0x10
};

// Additional format information for Windows.
struct QWindowsOpenGLAdditionalFormat
{
    QWindowsOpenGLAdditionalFormat(unsigned formatFlagsIn = 0, unsigned pixmapDepthIn = 0) :
        formatFlags(formatFlagsIn), pixmapDepth(pixmapDepthIn) {}
    unsigned formatFlags; // QWindowsGLFormatFlags.
    unsigned pixmapDepth; // for QWindowsGLRenderToPixmap
};

// Per-window data for active OpenGL contexts.
struct QOpenGLContextData
{
    QOpenGLContextData(HGLRC r, HWND h, HDC d) : renderingContext(r), hwnd(h), hdc(d) {}
    QOpenGLContextData() : renderingContext(0), hwnd(0), hdc(0) {}

    HGLRC renderingContext;
    HWND hwnd;
    HDC hdc;
};

class QOpenGLStaticContext
{
    Q_DISABLE_COPY(QOpenGLStaticContext)
    QOpenGLStaticContext();
public:
    enum Extensions
    {
        SampleBuffers = 0x1
    };

    typedef bool
        (APIENTRY *WglGetPixelFormatAttribIVARB)
            (HDC hdc, int iPixelFormat, int iLayerPlane,
             uint nAttributes, const int *piAttributes, int *piValues);

    typedef bool
        (APIENTRY *WglChoosePixelFormatARB)(HDC hdc, const int *piAttribList,
            const float *pfAttribFList, uint nMaxFormats, int *piFormats,
            UINT *nNumFormats);

    typedef HGLRC
        (APIENTRY *WglCreateContextAttribsARB)(HDC, HGLRC, const int *);

    bool hasExtensions() const
        { return wglGetPixelFormatAttribIVARB && wglChoosePixelFormatARB && wglCreateContextAttribsARB; }

    static QOpenGLStaticContext *create();
    static QByteArray getGlString(unsigned int which);

    const QByteArray vendor;
    const QByteArray renderer;
    const QByteArray extensionNames;
    int majorVersion;
    int minorVersion;
    unsigned extensions;

    WglGetPixelFormatAttribIVARB wglGetPixelFormatAttribIVARB;
    WglChoosePixelFormatARB wglChoosePixelFormatARB;
    WglCreateContextAttribsARB wglCreateContextAttribsARB;
};

QDebug operator<<(QDebug d, const QOpenGLStaticContext &);

class QWindowsGLContext : public QPlatformOpenGLContext
{
public:
    typedef QSharedPointer<QOpenGLStaticContext> QOpenGLStaticContextPtr;

    explicit QWindowsGLContext(const QOpenGLStaticContextPtr &staticContext,
                               QOpenGLContext *context);
    virtual ~QWindowsGLContext();
    bool isValid() const                  { return m_renderingContext; }
    virtual QSurfaceFormat format() const { return m_obtainedFormat; }

    virtual void swapBuffers(QPlatformSurface *surface);

    virtual bool makeCurrent(QPlatformSurface *surface);
    virtual void doneCurrent();

    typedef void (*GL_Proc) ();

    virtual GL_Proc getProcAddress(const QByteArray &procName);

    HGLRC renderingContext() const        { return m_renderingContext; }

private:
    inline void releaseDCs();

    const QOpenGLStaticContextPtr m_staticContext;
    QOpenGLContext *m_context;
    QSurfaceFormat m_obtainedFormat;
    HGLRC m_renderingContext;
    Array<QOpenGLContextData> m_windowContexts;
    PIXELFORMATDESCRIPTOR m_obtainedPixelFormatDescriptor;
    int m_pixelFormat;
    bool m_extensionsUsed;
};

QT_END_NAMESPACE

#endif // QWINDOWSGLCONTEXT_H

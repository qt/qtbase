/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtOpenVG module of the Qt Toolkit.
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

#ifndef QWINDOWSURFACE_VGEGL_P_H
#define QWINDOWSURFACE_VGEGL_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of the QLibrary class.  This header file may change from
// version to version without notice, or even be removed.
//
// We mean it.
//

#include <QtGui/private/qwindowsurface_p.h>
#include "qvg_p.h"

#if !defined(QT_NO_EGL)

#include <QtGui/private/qeglcontext_p.h>

QT_BEGIN_NAMESPACE

class QWindowSurface;

class Q_OPENVG_EXPORT QVGEGLWindowSurfacePrivate
{
public:
    QVGEGLWindowSurfacePrivate(QWindowSurface *win);
    virtual ~QVGEGLWindowSurfacePrivate();

    QVGPaintEngine *paintEngine();
    virtual QEglContext *ensureContext(QWidget *widget) = 0;
    virtual void beginPaint(QWidget *widget) = 0;
    virtual void endPaint
        (QWidget *widget, const QRegion& region, QImage *image = 0) = 0;
    virtual VGImage surfaceImage() const;
    virtual QSize surfaceSize() const = 0;
    virtual bool supportsStaticContents() const { return false; }
    virtual bool scroll(QWidget *, const QRegion&, int, int) { return false; }

private:
    QVGPaintEngine *engine;

protected:
    QWindowSurface *winSurface;

    void destroyPaintEngine();
    QSize windowSurfaceSize(QWidget *widget) const;
};

#if defined(EGL_OPENVG_IMAGE) && !defined(QVG_NO_SINGLE_CONTEXT)

#define QVG_VGIMAGE_BACKBUFFERS 1

class Q_OPENVG_EXPORT QVGEGLWindowSurfaceVGImage : public QVGEGLWindowSurfacePrivate
{
public:
    QVGEGLWindowSurfaceVGImage(QWindowSurface *win);
    virtual ~QVGEGLWindowSurfaceVGImage();

    QEglContext *ensureContext(QWidget *widget);
    void beginPaint(QWidget *widget);
    void endPaint(QWidget *widget, const QRegion& region, QImage *image);
    VGImage surfaceImage() const;
    QSize surfaceSize() const { return size; }

protected:
    QEglContext *context;
    VGImage backBuffer;
    EGLSurface backBufferSurface;
    bool recreateBackBuffer;
    bool isPaintingActive;
    QSize size;
    EGLSurface windowSurface;

    EGLSurface mainSurface() const;
};

#endif // EGL_OPENVG_IMAGE

class Q_OPENVG_EXPORT QVGEGLWindowSurfaceDirect : public QVGEGLWindowSurfacePrivate
{
public:
    QVGEGLWindowSurfaceDirect(QWindowSurface *win);
    virtual ~QVGEGLWindowSurfaceDirect();

    QEglContext *ensureContext(QWidget *widget);
    void beginPaint(QWidget *widget);
    void endPaint(QWidget *widget, const QRegion& region, QImage *image);
    QSize surfaceSize() const { return size; }
    bool supportsStaticContents() const;
    bool scroll(QWidget *widget, const QRegion& area, int dx, int dy);

protected:
    QEglContext *context;
    QSize size;
    bool isPaintingActive;
    bool needToSwap;
    EGLSurface windowSurface;
};

QT_END_NAMESPACE

#endif // !QT_NO_EGL

#endif // QWINDOWSURFACE_VGEGL_P_H

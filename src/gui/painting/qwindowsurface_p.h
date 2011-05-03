/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QWINDOWSURFACE_P_H
#define QWINDOWSURFACE_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of other Qt classes.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtGui/qwindow.h>

QT_BEGIN_NAMESPACE

class QPaintDevice;
class QRegion;
class QRect;
class QPoint;
class QImage;
class QWindowSurfacePrivate;
class QPlatformWindow;

class Q_GUI_EXPORT QWindowSurface
{
public:
    enum WindowSurfaceFeature {
        PartialUpdates               = 0x00000001, // Supports doing partial updates.
        PreservedContents            = 0x00000002, // Supports doing flush without first doing a repaint.
        StaticContents               = 0x00000004, // Supports having static content regions when being resized.
        AllFeatures                  = 0xffffffff  // For convenience
    };
    Q_DECLARE_FLAGS(WindowSurfaceFeatures, WindowSurfaceFeature)

    QWindowSurface(QWindow *window, bool setDefaultSurface = true);
    virtual ~QWindowSurface();

    QWindow *window() const;

    virtual QPaintDevice *paintDevice() = 0;

    // 'window' can be a child window, in which case 'region' is in child window coordinates and
    // offset is the (child) window's offset in relation to the window surface. On QWS, 'offset'
    // can be larger than just the offset from the top-level window as there may also be window
    // decorations which are painted into the window surface.
    virtual void flush(QWindow *window, const QRegion &region, const QPoint &offset) = 0;
#if !defined(Q_WS_QPA)
    virtual void setGeometry(const QRect &rect);
    QRect geometry() const;
#else
    virtual void resize(const QSize &size);
    QSize size() const;
    inline QRect geometry() const { return QRect(QPoint(), size()); }     //### cleanup before Qt 5
#endif

    virtual bool scroll(const QRegion &area, int dx, int dy);

    virtual void beginPaint(const QRegion &);
    virtual void endPaint(const QRegion &);

    virtual QPixmap grabWindow(const QWindow *window, const QRect& rectangle = QRect()) const;

    virtual QPoint offset(const QWindow *window) const;
    inline QRect rect(const QWindow *window) const;

    bool hasFeature(WindowSurfaceFeature feature) const;
    virtual WindowSurfaceFeatures features() const;

    void setStaticContents(const QRegion &region);
    QRegion staticContents() const;

protected:
    bool hasStaticContents() const;

private:
    QWindowSurfacePrivate *d_ptr;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QWindowSurface::WindowSurfaceFeatures)

inline QRect QWindowSurface::rect(const QWindow *window) const
{
    return window->geometry().translated(offset(window));
}

inline bool QWindowSurface::hasFeature(WindowSurfaceFeature feature) const
{
    return (features() & feature) != 0;
}

QT_END_NAMESPACE

#endif // QWINDOWSURFACE_P_H

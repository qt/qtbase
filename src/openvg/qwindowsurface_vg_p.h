/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtOpenVG module of the Qt Toolkit.
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

#ifndef QWINDOWSURFACE_VG_P_H
#define QWINDOWSURFACE_VG_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtGui/private/qwindowsurface_p.h>

QT_BEGIN_NAMESPACE

#if !defined(QT_NO_EGL)

class QVGEGLWindowSurfacePrivate;

class Q_OPENVG_EXPORT QVGWindowSurface : public QWindowSurface, public QPaintDevice
{
public:
    QVGWindowSurface(QWidget *window);
    QVGWindowSurface(QWidget *window, QVGEGLWindowSurfacePrivate *d);
    ~QVGWindowSurface();

    QPaintDevice *paintDevice();
    void flush(QWidget *widget, const QRegion &region, const QPoint &offset);
    void setGeometry(const QRect &rect);
    bool scroll(const QRegion &area, int dx, int dy);

    void beginPaint(const QRegion &region);
    void endPaint(const QRegion &region);

    QPaintEngine *paintEngine() const;

    WindowSurfaceFeatures features() const;

protected:
    int metric(PaintDeviceMetric metric) const;

private:
    QVGEGLWindowSurfacePrivate *d_ptr;
};

#endif

QT_END_NAMESPACE

#endif // QWINDOWSURFACE_VG_P_H

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

#ifndef QUNIFIEDTOOLBARSURFACE_MAC_P_H
#define QUNIFIEDTOOLBARSURFACE_MAC_P_H

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

#include <private/qwindowsurface_raster_p.h>
#include <QWidget>
#include <QToolBar>
#include <private/qwidget_p.h>
#include <private/qnativeimage_p.h>

#ifdef QT_MAC_USE_COCOA

QT_BEGIN_NAMESPACE

class QNativeImage;


class QUnifiedToolbarSurfacePrivate
{
public:
    QNativeImage *image;
    uint inSetGeometry : 1;
};

class Q_GUI_EXPORT QUnifiedToolbarSurface : public QRasterWindowSurface
{
public:
    QUnifiedToolbarSurface(QWidget *widget);
    ~QUnifiedToolbarSurface();

    void flush(QWidget *widget);
    void flush(QWidget *widget, const QRegion &region, const QPoint &offset);
    void setGeometry(const QRect &rect);
    void beginPaint(const QRegion &rgn);
    void insertToolbar(QWidget *toolbar, const QPoint &offset);
    void removeToolbar(QToolBar *toolbar);
    void updateToolbarOffset(QWidget *widget);
    void renderToolbar(QWidget *widget, bool forceFlush = false);
    void recursiveRedirect(QObject *widget, QWidget *parent_toolbar, const QPoint &offset);

    QPaintDevice *paintDevice();
    CGContextRef imageContext();

private:
    void prepareBuffer(QImage::Format format, QWidget *widget);
    void recursiveRemoval(QObject *object);

    Q_DECLARE_PRIVATE(QUnifiedToolbarSurface)
    QScopedPointer<QUnifiedToolbarSurfacePrivate> d_ptr;
};

QT_END_NAMESPACE

#endif // QT_MAC_USE_COCOA

#endif // QUNIFIEDTOOLBARSURFACE_MAC_P_H

/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the plugins of the Qt Toolkit.
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


#include "qminimalwindowsurface.h"
#include <QtCore/qdebug.h>
#include <QtGui/private/qapplication_p.h>

QT_BEGIN_NAMESPACE

QMinimalWindowSurface::QMinimalWindowSurface(QWindow *window)
    : QWindowSurface(window)
{
    //qDebug() << "QMinimalWindowSurface::QMinimalWindowSurface:" << (long)this;
}

QMinimalWindowSurface::~QMinimalWindowSurface()
{
}

QPaintDevice *QMinimalWindowSurface::paintDevice()
{
    //qDebug() << "QMinimalWindowSurface::paintDevice";
    return &mImage;
}

void QMinimalWindowSurface::flush(QWindow *window, const QRegion &region, const QPoint &offset)
{
    Q_UNUSED(window);
    Q_UNUSED(region);
    Q_UNUSED(offset);

    static int c = 0;
    QString filename = QString("output%1.png").arg(c++, 4, 10, QLatin1Char('0'));
    qDebug() << "QMinimalWindowSurface::flush() saving contents to" << filename.toLocal8Bit().constData();
    mImage.save(filename);
}

void QMinimalWindowSurface::resize(const QSize &size)
{
    //qDebug() << "QMinimalWindowSurface::setGeometry:" << (long)this << rect;
    QWindowSurface::resize(size);
    QImage::Format format = QApplicationPrivate::platformIntegration()->screens().first()->format();
    if (mImage.size() != size)
        mImage = QImage(size, format);
}

QT_END_NAMESPACE

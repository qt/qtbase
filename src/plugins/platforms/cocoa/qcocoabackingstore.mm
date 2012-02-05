/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
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
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qcocoabackingstore.h"
#include "qcocoaautoreleasepool.h"

#include <QtCore/qdebug.h>
#include <QtGui/QPainter>

QT_BEGIN_NAMESPACE

QRect flipedRect(const QRect &sourceRect,int height)
{
    if (!sourceRect.isValid())
        return QRect();
    QRect flippedRect = sourceRect;
    flippedRect.moveTop(height - sourceRect.y());
    return flippedRect;
}

QCocoaBackingStore::QCocoaBackingStore(QWindow *window)
    : QPlatformBackingStore(window)
{
    m_cocoaWindow = static_cast<QCocoaWindow *>(window->handle());

    const QRect geo = window->geometry();
    NSRect rect = NSMakeRect(geo.x(),geo.y(),geo.width(),geo.height());

    m_image = new QImage(window->geometry().size(),QImage::Format_ARGB32_Premultiplied);
}

QCocoaBackingStore::~QCocoaBackingStore()
{
    delete m_image;
}

QPaintDevice *QCocoaBackingStore::paintDevice()
{
    return m_image;
}

void QCocoaBackingStore::flush(QWindow *widget, const QRegion &region, const QPoint &offset)
{
    Q_UNUSED(widget);
    Q_UNUSED(offset);
    QCocoaAutoReleasePool pool;

    QRect geo = region.boundingRect();

    NSRect rect = NSMakeRect(geo.x(), geo.y(), geo.width(), geo.height());
    [m_cocoaWindow->m_contentView displayRect:rect];
}

void QCocoaBackingStore::resize(const QSize &size, const QRegion &)
{
    delete m_image;
    m_image = new QImage(size,QImage::Format_ARGB32_Premultiplied);
    NSSize newSize = NSMakeSize(size.width(),size.height());
    [static_cast<QNSView *>(m_cocoaWindow->m_contentView) setImage:m_image];
}

QT_END_NAMESPACE

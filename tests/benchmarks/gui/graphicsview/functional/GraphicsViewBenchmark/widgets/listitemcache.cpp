/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the examples of the Qt Toolkit.
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

#include <QGraphicsItem>
#include <QPainter>
#include <QDebug>
#include "listitemcache.h"

ListItemCache::ListItemCache()
{
}

ListItemCache::~ListItemCache()
{
    QPixmapCache::remove(m_cacheKey);
}

void ListItemCache::draw(QPainter * painter) 
{
    QRectF irect = sourceBoundingRect(Qt::LogicalCoordinates);
    QRectF vrect = painter->clipPath().boundingRect();

    if (vrect.intersects(irect)) {
        QRectF newVisibleRect = irect.intersected(vrect);
        QPixmap pixmap;

        if (!QPixmapCache::find(m_cacheKey, &pixmap) ||
            m_visibleRect.toRect() != newVisibleRect.toRect()) {
            //qDebug() << "ListItemCache: caching" << m_visibleRect
            //    << "->" << newVisibleRect;

            pixmap = QPixmap(sourceBoundingRect().toRect().size());
            pixmap.fill(Qt::transparent);

            QPainter pixmapPainter(&pixmap);
            drawSource(&pixmapPainter);
            pixmapPainter.end();
            m_cacheKey = QPixmapCache::insert(pixmap);

            m_visibleRect = newVisibleRect;
        }

        //qDebug() << "ListItemCache: blitting" << m_visibleRect;
        painter->drawPixmap(0, 0, pixmap);
    } 
}

void ListItemCache::sourceChanged(ChangeFlags)
{
    QPixmapCache::remove(m_cacheKey);
}




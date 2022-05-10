// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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




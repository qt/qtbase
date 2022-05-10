// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef LISTITEMCACHE_H
#define LISTITEMCACHE_H

#include <QGraphicsEffect>
#include <QPixmapCache>

class ListItemCache : public QGraphicsEffect
{
    Q_OBJECT

public:

    ListItemCache();
    ~ListItemCache();

public: // QGraphicsEffect

    void draw(QPainter *painter);
    void sourceChanged(ChangeFlags flags);

private:

    QPixmapCache::Key m_cacheKey;
    QRectF m_visibleRect;
};

#endif // LISTITEMCACHE_H


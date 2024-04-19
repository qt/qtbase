// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QSTYLE_P_H
#define QSTYLE_P_H

#include "private/qobject_p.h"
#include <QtGui/qguiapplication.h>
#include <QtWidgets/qstyle.h>

QT_BEGIN_NAMESPACE

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of qstyle_*.cpp.  This header file may change from version to version
// without notice, or even be removed.
//
// We mean it.
//

// Private class

class QStyle;

class QStylePrivate: public QObjectPrivate
{
    Q_DECLARE_PUBLIC(QStyle)
public:
    static bool useFullScreenForPopup();

    QStyle *proxyStyle;
    QString name;
};

inline QPixmap styleCachePixmap(const QSize &size, qreal pixelRatio)
{
    QPixmap cachePixmap = QPixmap(size * pixelRatio);
    cachePixmap.setDevicePixelRatio(pixelRatio);
    cachePixmap.fill(Qt::transparent);
    return cachePixmap;
}

#define BEGIN_STYLE_PIXMAPCACHE(a) \
    QRect rect = option->rect; \
    QPixmap internalPixmapCache; \
    QPainter *p = painter; \
    const auto dpr = p->device()->devicePixelRatio(); \
    const QString unique = QStyleHelper::uniqueName((a), option, option->rect.size(), dpr); \
    int txType = painter->deviceTransform().type() | painter->worldTransform().type(); \
    const bool doPixmapCache = (!option->rect.isEmpty()) \
            && ((txType <= QTransform::TxTranslate) || (painter->deviceTransform().type() == QTransform::TxScale)); \
    if (doPixmapCache && QPixmapCache::find(unique, &internalPixmapCache)) { \
        painter->drawPixmap(option->rect.topLeft(), internalPixmapCache); \
    } else { \
        if (doPixmapCache) { \
            rect.setRect(0, 0, option->rect.width(), option->rect.height()); \
            internalPixmapCache = styleCachePixmap(option->rect.size(), dpr); \
            p = new QPainter(&internalPixmapCache); \
        }



#define END_STYLE_PIXMAPCACHE \
        if (doPixmapCache) { \
            p->end(); \
            delete p; \
            painter->drawPixmap(option->rect.topLeft(), internalPixmapCache); \
            QPixmapCache::insert(unique, internalPixmapCache); \
        } \
    }

QT_END_NAMESPACE

#endif //QSTYLE_P_H

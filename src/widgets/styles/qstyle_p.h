/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtWidgets module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

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
    inline QStylePrivate()
        : layoutSpacingIndex(-1), proxyStyle(nullptr) {}

    static bool useFullScreenForPopup();

    mutable int layoutSpacingIndex;
    QStyle *proxyStyle;
};

inline QImage styleCacheImage(const QSize &size)
{
    const qreal pixelRatio = qApp->devicePixelRatio();
    QImage cacheImage = QImage(size * pixelRatio, QImage::Format_ARGB32_Premultiplied);
    cacheImage.setDevicePixelRatio(pixelRatio);
    return cacheImage;
}

inline QPixmap styleCachePixmap(const QSize &size)
{
    const qreal pixelRatio = qApp->devicePixelRatio();
    QPixmap cachePixmap = QPixmap(size * pixelRatio);
    cachePixmap.setDevicePixelRatio(pixelRatio);
    return cachePixmap;
}

#define BEGIN_STYLE_PIXMAPCACHE(a) \
    QRect rect = option->rect; \
    QPixmap internalPixmapCache; \
    QImage imageCache; \
    QPainter *p = painter; \
    QString unique = QStyleHelper::uniqueName((a), option, option->rect.size()); \
    int txType = painter->deviceTransform().type() | painter->worldTransform().type(); \
    bool doPixmapCache = (!option->rect.isEmpty()) \
            && ((txType <= QTransform::TxTranslate) || (painter->deviceTransform().type() == QTransform::TxScale)); \
    if (doPixmapCache && QPixmapCache::find(unique, &internalPixmapCache)) { \
        painter->drawPixmap(option->rect.topLeft(), internalPixmapCache); \
    } else { \
        if (doPixmapCache) { \
            rect.setRect(0, 0, option->rect.width(), option->rect.height()); \
            imageCache = styleCacheImage(option->rect.size()); \
            imageCache.fill(0); \
            p = new QPainter(&imageCache); \
        }



#define END_STYLE_PIXMAPCACHE \
        if (doPixmapCache) { \
            p->end(); \
            delete p; \
            internalPixmapCache = QPixmap::fromImage(imageCache); \
            painter->drawPixmap(option->rect.topLeft(), internalPixmapCache); \
            QPixmapCache::insert(unique, internalPixmapCache); \
        } \
    }

QT_END_NAMESPACE

#endif //QSTYLE_P_H

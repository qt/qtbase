// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QANDROIDPLATFORMICONENGINE_H
#define QANDROIDPLATFORMICONENGINE_H

#include <QtGui/qiconengine.h>
#include <QtGui/qfont.h>

QT_BEGIN_NAMESPACE

class QAndroidPlatformIconEngine : public QIconEngine
{
public:
    QAndroidPlatformIconEngine(const QString &iconName);
    ~QAndroidPlatformIconEngine();
    QIconEngine *clone() const override;
    QString key() const override;
    QString iconName() override;
    bool isNull() override;

    QList<QSize> availableSizes(QIcon::Mode, QIcon::State) override;
    QSize actualSize(const QSize &size, QIcon::Mode mode, QIcon::State state) override;
    QPixmap pixmap(const QSize &size, QIcon::Mode mode, QIcon::State state) override;
    QPixmap scaledPixmap(const QSize &size, QIcon::Mode mode, QIcon::State state, qreal scale) override;
    void paint(QPainter *painter, const QRect &rect, QIcon::Mode mode, QIcon::State state) override;

private:
    static constexpr quint64 calculateCacheKey(QIcon::Mode mode, QIcon::State state)
    {
        return (quint64(mode) << 32) | state;
    }
    QString glyphs() const;

    const QString m_iconName;
    QFont m_iconFont;
    const QString m_glyphs;
    mutable QPixmap m_pixmap;
    mutable quint64 m_cacheKey = {};
};

QT_END_NAMESPACE

#endif // QANDROIDPLATFORMICONENGINE_H

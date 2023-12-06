// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QWINDOWSICONENGINE_H
#define QWINDOWSICONENGINE_H

#include <QtCore/qt_windows.h>

#include <QtGui/qfont.h>
#include <QtGui/qiconengine.h>

QT_BEGIN_NAMESPACE

class QWindowsIconEngine : public QIconEngine
{
public:
    QWindowsIconEngine(const QString &iconName);
    ~QWindowsIconEngine();
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
    const QFont m_iconFont;
    const QString m_glyphs;
    mutable QPixmap m_pixmap;
    mutable quint64 m_cacheKey = {};
};

QT_END_NAMESPACE

#endif // QWINDOWSICONENGINE_H

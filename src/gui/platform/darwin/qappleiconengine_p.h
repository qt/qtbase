// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QAPPLEICONENGINE_P_H
#define QAPPLEICONENGINE_P_H

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

#include <QtGui/qiconengine.h>

#include <QtCore/private/qcore_mac_p.h>

Q_FORWARD_DECLARE_OBJC_CLASS(UIImage);
Q_FORWARD_DECLARE_OBJC_CLASS(NSImage);

QT_BEGIN_NAMESPACE

class Q_GUI_EXPORT QAppleIconEngine : public QIconEngine
{
public:
    QAppleIconEngine(const QString &iconName);
    ~QAppleIconEngine();
    QIconEngine *clone() const override;
    QString key() const override;
    QString iconName() override;
    bool isNull() override;

    QList<QSize> availableSizes(QIcon::Mode, QIcon::State) override;
    QSize actualSize(const QSize &size, QIcon::Mode mode, QIcon::State state) override;
    QPixmap pixmap(const QSize &size, QIcon::Mode mode, QIcon::State state) override;
    QPixmap scaledPixmap(const QSize &size, QIcon::Mode mode, QIcon::State state, qreal scale) override;
    void paint(QPainter *painter, const QRect &rect, QIcon::Mode mode, QIcon::State state) override;

    static QList<QSize> availableIconSizes(double aspectRatio = 1.0);

private:
    static constexpr quint64 calculateCacheKey(QIcon::Mode mode, QIcon::State state)
    {
        return (quint64(mode) << 32) | state;
    }

    const QString m_iconName;
#if defined(Q_OS_MACOS)
    const NSImage *m_image;
#elif defined(Q_OS_IOS)
    const UIImage *m_image;
#endif
    mutable QPixmap m_pixmap;
    mutable quint64 m_cacheKey = {};
};


QT_END_NAMESPACE

#endif // QAPPLEICONENGINE_P_H

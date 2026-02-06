// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QANDROIDPLATFORMFILEICONENGINE_H
#define QANDROIDPLATFORMFILEICONENGINE_H

#include <QtCore/qjnitypes.h>

#include <QtGui/qpixmap.h>
#include <QtGui/private/qabstractfileiconengine_p.h>
#include "qandroidplatformiconengine.h"

#ifndef QT_NO_ICON

QT_BEGIN_NAMESPACE

Q_DECLARE_JNI_CLASS(Drawable, "android/graphics/drawable/Drawable")

class QAndroidPlatformFileIconEngine : public QAbstractFileIconEngine
{
public:
    explicit QAndroidPlatformFileIconEngine(const QFileInfo &info, QPlatformTheme::IconOptions opts);
    ~QAndroidPlatformFileIconEngine();

    QList<QSize> availableSizes(QIcon::Mode = QIcon::Normal, QIcon::State = QIcon::Off) override
    { return {{16, 16}, {24, 24}, {48, 48}, {128, 128}}; }

    bool isNull() override;

protected:
    QPixmap filePixmap(const QSize &size, QIcon::Mode, QIcon::State) override;

private:
    std::optional<QtJniTypes::Drawable> m_drawable;
    QPixmap m_pixmap;
};

QT_END_NAMESPACE

#endif // QT_NO_ICON

#endif // QANDROIDPLATFORMFILEICONENGINE_H

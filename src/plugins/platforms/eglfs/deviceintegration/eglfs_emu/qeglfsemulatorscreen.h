// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QEGLFSEMULATORSCREEN_H
#define QEGLFSEMULATORSCREEN_H

#include <QtCore/QJsonObject>

#include "qeglfsemulatorintegration.h"
#include "private/qeglfsscreen_p.h"

QT_BEGIN_NAMESPACE

class QEglFSEmulatorScreen : public QEglFSScreen
{
public:
    QEglFSEmulatorScreen(const QJsonObject &screenDescription);

    QRect geometry() const override;
    QRect rawGeometry() const override;
    int depth() const override;
    QImage::Format format() const override;
    QSizeF physicalSize() const override;
    QDpi logicalDpi() const override;
    QDpi logicalBaseDpi() const override;
    qreal refreshRate() const override;
    Qt::ScreenOrientation nativeOrientation() const override;
    Qt::ScreenOrientation orientation() const override;
    QString name() const override;

    uint id() const;

private:
    void initFromJsonObject(const QJsonObject &description);

    QString m_description;
    QRect m_geometry;
    int m_depth;
    QImage::Format m_format;
    QSizeF m_physicalSize;
    float m_refreshRate;
    Qt::ScreenOrientation m_nativeOrientation;
    Qt::ScreenOrientation m_orientation;
    uint m_id;
};

QT_END_NAMESPACE

#endif // QEGLFSEMULATORSCREEN_H

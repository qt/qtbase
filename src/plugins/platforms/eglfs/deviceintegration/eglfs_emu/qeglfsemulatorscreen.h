/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
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
    qreal pixelDensity() const override;
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
    float m_pixelDensity;
    float m_refreshRate;
    Qt::ScreenOrientation m_nativeOrientation;
    Qt::ScreenOrientation m_orientation;
    uint m_id;
};

QT_END_NAMESPACE

#endif // QEGLFSEMULATORSCREEN_H

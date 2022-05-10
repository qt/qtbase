// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QSCREEN_P_H
#define QSCREEN_P_H

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

#include <QtGui/private/qtguiglobal_p.h>
#include <QtGui/qscreen.h>
#include <qpa/qplatformscreen.h>
#include "qhighdpiscaling_p.h"

#include <QtCore/private/qobject_p.h>

QT_BEGIN_NAMESPACE

class QScreenPrivate : public QObjectPrivate
{
    Q_DECLARE_PUBLIC(QScreen)
public:
    void setPlatformScreen(QPlatformScreen *screen);
    void updateHighDpi()
    {
        geometry = platformScreen->deviceIndependentGeometry();
        availableGeometry = QHighDpi::fromNative(platformScreen->availableGeometry(), QHighDpiScaling::factor(platformScreen), geometry.topLeft());
    }

    void updatePrimaryOrientation();
    void updateGeometriesWithSignals();
    void emitGeometryChangeSignals(bool geometryChanged, bool availableGeometryChanged);

    QPlatformScreen *platformScreen = nullptr;

    Qt::ScreenOrientation orientation = Qt::PrimaryOrientation;
    Qt::ScreenOrientation primaryOrientation = Qt::LandscapeOrientation;
    QRect geometry;
    QRect availableGeometry;
    QDpi logicalDpi = {96, 96};
    qreal refreshRate = 60;
};

QT_END_NAMESPACE

#endif // QSCREEN_P_H

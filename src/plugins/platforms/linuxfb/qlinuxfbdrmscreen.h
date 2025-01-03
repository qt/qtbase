// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QLINUXFBDRMSCREEN_H
#define QLINUXFBDRMSCREEN_H

#include <QtFbSupport/private/qfbscreen_p.h>

QT_BEGIN_NAMESPACE

class QKmsScreenConfig;
class QLinuxFbDevice;

class QLinuxFbDrmScreen : public QFbScreen
{
    Q_OBJECT
public:
    QLinuxFbDrmScreen(const QStringList &args);
    ~QLinuxFbDrmScreen();

    bool initialize() override;
    QRegion doRedraw() override;
    QPixmap grabWindow(WId wid, int x, int y, int width, int height) const override;

private:
    QKmsScreenConfig *m_screenConfig;
    QLinuxFbDevice *m_device;
};

QT_END_NAMESPACE

#endif // QLINUXFBDRMSCREEN_H

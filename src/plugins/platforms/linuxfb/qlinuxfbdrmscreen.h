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

/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Copyright (C) 2015-2016 Oleksandr Tymoshenko <gonzo@bluezbox.com>
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

#ifndef QBSDFBSCREEN_H
#define QBSDFBSCREEN_H

#include <QtFbSupport/private/qfbscreen_p.h>

QT_BEGIN_NAMESPACE

class QPainter;

class QBsdFbScreen : public QFbScreen
{
    Q_OBJECT
public:
    explicit QBsdFbScreen(const QStringList &args);
    ~QBsdFbScreen() override;

    bool initialize() override;

    QPixmap grabWindow(WId wid, int x, int y, int width, int height) const override;

    QRegion doRedraw() override;

private:
    QStringList m_arguments;
    int m_framebufferFd = -1;
    QImage m_onscreenImage;

    int m_bytesPerLine = -1;

    struct {
        uchar *data;
        int offset, size;
    } m_mmap;

    QScopedPointer<QPainter> m_blitter;
};

QT_END_NAMESPACE

#endif // QBSDFBSCREEN_H

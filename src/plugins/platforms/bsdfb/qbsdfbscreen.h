// Copyright (C) 2017 The Qt Company Ltd.
// Copyright (C) 2015-2016 Oleksandr Tymoshenko <gonzo@bluezbox.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

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

// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QLINUXFBSCREEN_H
#define QLINUXFBSCREEN_H

#include <QtFbSupport/private/qfbscreen_p.h>

QT_BEGIN_NAMESPACE

class QPainter;
class QFbCursor;

class QLinuxFbScreen : public QFbScreen
{
    Q_OBJECT
public:
    QLinuxFbScreen(const QStringList &args);
    ~QLinuxFbScreen();

    bool initialize() override;

    QPixmap grabWindow(WId wid, int x, int y, int width, int height) const override;

    QRegion doRedraw() override;

private:
    QStringList mArgs;
    int mFbFd;
    int mTtyFd;

    QImage mFbScreenImage;
    int mBytesPerLine;
    int mOldTtyMode;

    struct {
        uchar *data;
        int offset, size;
    } mMmap;

    QPainter *mBlitter;
};

QT_END_NAMESPACE

#endif // QLINUXFBSCREEN_H


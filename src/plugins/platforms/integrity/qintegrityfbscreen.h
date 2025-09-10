// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QINTEGRITYFBSCREEN_H
#define QINTEGRITYFBSCREEN_H

#include <QtFbSupport/private/qfbscreen_p.h>
#include <device/fbdriver.h>

QT_BEGIN_NAMESPACE

class QPainter;
class QFbCursor;

class QIntegrityFbScreen : public QFbScreen
{
    Q_OBJECT
public:
    QIntegrityFbScreen(const QStringList &args);
    ~QIntegrityFbScreen();

    bool initialize();

    QPixmap grabWindow(WId wid, int x, int y, int width, int height) const override;

    QRegion doRedraw() override;

private:
    QStringList mArgs;
    FBDriver *mFbd;
    FBHandle mFbh;
    FBInfo mFbinfo;
    MemoryRegion mVMR;
    Address mVMRCookie;

    QImage mFbScreenImage;

    QPainter *mBlitter;
};

QT_END_NAMESPACE

#endif // QINTEGRITYFBSCREEN_H


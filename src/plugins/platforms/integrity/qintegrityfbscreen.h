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


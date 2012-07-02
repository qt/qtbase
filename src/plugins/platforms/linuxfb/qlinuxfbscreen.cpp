/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qlinuxfbscreen.h"
#include <QtPlatformSupport/private/qfbcursor_p.h>
#include <QtGui/QPainter>

QT_BEGIN_NAMESPACE

QLinuxFbScreen::QLinuxFbScreen(uchar * d, int w,
    int h, int lstep, QImage::Format screenFormat) : compositePainter(0)
{
    data = d;
    mGeometry = QRect(0,0,w,h);
    bytesPerLine = lstep;
    mFormat = screenFormat;
    mDepth = 16;
    mScreenImage = new QImage(mGeometry.width(), mGeometry.height(),
                              mFormat);
    mFbScreenImage = new QImage(data, mGeometry.width(), mGeometry.height(),
                              bytesPerLine, mFormat);
    cursor = new QFbCursor(this);
}

void QLinuxFbScreen::setGeometry(QRect rect)
{
    mGeometry = rect;
    delete mFbScreenImage;
    mFbScreenImage = new QImage(data, mGeometry.width(), mGeometry.height(),
                           bytesPerLine, mFormat);
    delete compositePainter;
    compositePainter = 0;

    delete mScreenImage;
    mScreenImage = new QImage(mGeometry.width(), mGeometry.height(),
                              mFormat);
}

void QLinuxFbScreen::setFormat(QImage::Format format)
{
    mFormat = format;
    delete mFbScreenImage;
    mFbScreenImage = new QImage(data, mGeometry.width(), mGeometry.height(),
                             bytesPerLine, mFormat);
    delete compositePainter;
    compositePainter = 0;

    delete mScreenImage;
    mScreenImage = new QImage(mGeometry.width(), mGeometry.height(),
                              mFormat);
}

QRegion QLinuxFbScreen::doRedraw()
{
    QRegion touched;
    touched = QFbScreen::doRedraw();

    if (!compositePainter) {
        compositePainter = new QPainter(mFbScreenImage);
    }

    QVector<QRect> rects = touched.rects();
    for (int i = 0; i < rects.size(); i++)
        compositePainter->drawImage(rects[i], *mScreenImage, rects[i]);
    return touched;
}

QT_END_NAMESPACE


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

#include "qfbbackingstore_p.h"
#include "qfbwindow_p.h"
#include "qfbscreen_p.h"

#include <qpa/qplatformwindow.h>
#include <QtGui/qscreen.h>
#include <QtGui/qpainter.h>

QT_BEGIN_NAMESPACE

QFbBackingStore::QFbBackingStore(QWindow *window)
    : QPlatformBackingStore(window)
{
    if (window->handle())
        (static_cast<QFbWindow *>(window->handle()))->setBackingStore(this);
    else
        (static_cast<QFbScreen *>(window->screen()->handle()))->addPendingBackingStore(this);
}

QFbBackingStore::~QFbBackingStore()
{
}

void QFbBackingStore::flush(QWindow *window, const QRegion &region, const QPoint &offset)
{
    Q_UNUSED(window);
    Q_UNUSED(offset);

    (static_cast<QFbWindow *>(window->handle()))->repaint(region);
}

void QFbBackingStore::resize(const QSize &size, const QRegion &staticContents)
{
    Q_UNUSED(staticContents);

    if (mImage.size() != size)
        mImage = QImage(size, window()->screen()->handle()->format());
}

const QImage QFbBackingStore::image()
{
    return mImage;
}


QImage QFbBackingStore::toImage() const
{
    return mImage;
}

void QFbBackingStore::lock()
{
    mImageMutex.lock();
}

void QFbBackingStore::unlock()
{
    mImageMutex.unlock();
}

void QFbBackingStore::beginPaint(const QRegion &region)
{
    lock();

    if (mImage.hasAlphaChannel()) {
        QPainter p(&mImage);
        p.setCompositionMode(QPainter::CompositionMode_Source);
        for (const QRect &r : region)
            p.fillRect(r, Qt::transparent);
    }
}

void QFbBackingStore::endPaint()
{
    unlock();
}

QT_END_NAMESPACE

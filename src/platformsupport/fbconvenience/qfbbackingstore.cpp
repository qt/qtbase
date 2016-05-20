/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
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
        const QVector<QRect> rects = region.rects();
        for (QVector<QRect>::const_iterator it = rects.begin(); it != rects.end(); ++it)
            p.fillRect(*it, Qt::transparent);
    }
}

void QFbBackingStore::endPaint()
{
    unlock();
}

QT_END_NAMESPACE


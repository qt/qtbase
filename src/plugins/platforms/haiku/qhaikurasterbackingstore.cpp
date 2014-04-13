/***************************************************************************
**
** Copyright (C) 2015 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Tobias Koenig <tobias.koenig@kdab.com>
** Contact: http://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qhaikurasterbackingstore.h"
#include "qhaikurasterwindow.h"

#include <Bitmap.h>
#include <View.h>

QT_BEGIN_NAMESPACE

QHaikuRasterBackingStore::QHaikuRasterBackingStore(QWindow *window)
    : QPlatformBackingStore(window)
    , m_bitmap(Q_NULLPTR)
{
}

QHaikuRasterBackingStore::~QHaikuRasterBackingStore()
{
    delete m_bitmap;
    m_bitmap = Q_NULLPTR;
}

QPaintDevice *QHaikuRasterBackingStore::paintDevice()
{
    if (!m_bufferSize.isEmpty() && m_bitmap)
        return m_buffer.image();

    return Q_NULLPTR;
}

void QHaikuRasterBackingStore::flush(QWindow *window, const QRegion &region, const QPoint &offset)
{
    Q_UNUSED(region);
    Q_UNUSED(offset);

    if (!window)
        return;

    QHaikuRasterWindow *targetWindow = static_cast<QHaikuRasterWindow*>(window->handle());

    BView *view = targetWindow->nativeViewHandle();

    view->LockLooper();
    view->DrawBitmap(m_bitmap);
    view->UnlockLooper();
}

void QHaikuRasterBackingStore::resize(const QSize &size, const QRegion &staticContents)
{
    Q_UNUSED(staticContents);

    if (m_bufferSize == size)
        return;

    delete m_bitmap;
    m_bitmap = new BBitmap(BRect(0, 0, size.width(), size.height()), B_RGB32, false, true);
    m_buffer = QHaikuBuffer(m_bitmap);
    m_bufferSize = size;
}

QT_END_NAMESPACE

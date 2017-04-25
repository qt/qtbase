/***************************************************************************
**
** Copyright (C) 2011 - 2013 BlackBerry Limited. All rights reserved.
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

#include "qqnxrasterbackingstore.h"
#include "qqnxrasterwindow.h"
#include "qqnxscreen.h"
#include "qqnxglobal.h"

#include <QtCore/QDebug>

#include <errno.h>

#if defined(QQNXRASTERBACKINGSTORE_DEBUG)
#define qRasterBackingStoreDebug qDebug
#else
#define qRasterBackingStoreDebug QT_NO_QDEBUG_MACRO
#endif

QT_BEGIN_NAMESPACE

QQnxRasterBackingStore::QQnxRasterBackingStore(QWindow *window)
    : QPlatformBackingStore(window),
      m_needsPosting(false),
      m_scrolled(false)
{
    qRasterBackingStoreDebug() << "w =" << window;

    m_window = window;
}

QQnxRasterBackingStore::~QQnxRasterBackingStore()
{
    qRasterBackingStoreDebug() << "w =" << window();
}

QPaintDevice *QQnxRasterBackingStore::paintDevice()
{
    if (platformWindow() && platformWindow()->hasBuffers())
        return platformWindow()->renderBuffer().image();

    return 0;
}

void QQnxRasterBackingStore::flush(QWindow *window, const QRegion &region, const QPoint &offset)
{
    Q_UNUSED(offset)

    qRasterBackingStoreDebug() << "w =" << this->window();

    // Sometimes this method is called even though there is nothing to be
    // flushed (posted in "screen" parlance), for instance, after an expose
    // event directly follows a geometry change event.
    if (!m_needsPosting)
        return;

    QQnxWindow *targetWindow = 0;
    if (window)
        targetWindow = static_cast<QQnxWindow *>(window->handle());

    // we only need to flush the platformWindow backing store, since this is
    // the buffer where all drawing operations of all windows, including the
    // child windows, are performed; conceptually ,child windows have no buffers
    // (actually they do have a 1x1 placeholder buffer due to libscreen limitations),
    // since Qt will only draw to the backing store of the top-level window.
    if (!targetWindow || targetWindow == platformWindow())
        platformWindow()->post(region);  // update the display with newly rendered content

    m_needsPosting = false;
    m_scrolled = false;
}

void QQnxRasterBackingStore::resize(const QSize &size, const QRegion &staticContents)
{
    Q_UNUSED(size);
    Q_UNUSED(staticContents);
    qRasterBackingStoreDebug() << "w =" << window() << ", s =" << size;

    // NOTE: defer resizing window buffers until next paint as
    // resize() can be called multiple times before a paint occurs
}

bool QQnxRasterBackingStore::scroll(const QRegion &area, int dx, int dy)
{
    qRasterBackingStoreDebug() << "w =" << window();

    m_needsPosting = true;

    if (!m_scrolled) {
        platformWindow()->scroll(area, dx, dy, true);
        m_scrolled = true;
        return true;
    }
    return false;
}

void QQnxRasterBackingStore::beginPaint(const QRegion &region)
{
    Q_UNUSED(region);

    qRasterBackingStoreDebug() << "w =" << window();
    m_needsPosting = true;

    platformWindow()->adjustBufferSize();

    if (window()->requestedFormat().alphaBufferSize() > 0) {
        auto platformScreen = static_cast<QQnxScreen *>(platformWindow()->screen());
        for (const QRect &r : region) {
            // Clear transparent regions
            const int bg[] = {
                               SCREEN_BLIT_COLOR, 0x00000000,
                               SCREEN_BLIT_DESTINATION_X, r.x(),
                               SCREEN_BLIT_DESTINATION_Y, r.y(),
                               SCREEN_BLIT_DESTINATION_WIDTH, r.width(),
                               SCREEN_BLIT_DESTINATION_HEIGHT, r.height(),
                               SCREEN_BLIT_END
                              };
            Q_SCREEN_CHECKERROR(screen_fill(platformScreen->nativeContext(),
                                            platformWindow()->renderBuffer().nativeBuffer(), bg),
                                "failed to clear transparent regions");
        }
        Q_SCREEN_CHECKERROR(screen_flush_blits(platformScreen->nativeContext(),
                    SCREEN_WAIT_IDLE), "failed to flush blits");
    }
}

void QQnxRasterBackingStore::endPaint()
{
    qRasterBackingStoreDebug() << "w =" << window();
}

QQnxRasterWindow *QQnxRasterBackingStore::platformWindow() const
{
  Q_ASSERT(m_window->handle());
  return static_cast<QQnxRasterWindow*>(m_window->handle());
}

QT_END_NAMESPACE

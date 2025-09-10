// Copyright (C) 2011 - 2013 BlackBerry Limited. All rights reserved.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qqnxrasterbackingstore.h"
#include "qqnxrasterwindow.h"
#include "qqnxscreen.h"
#include "qqnxglobal.h"

#include <QtCore/QDebug>
#include <QtGui/QBackingStore>

#include <errno.h>

QT_BEGIN_NAMESPACE

QQnxRasterBackingStore::QQnxRasterBackingStore(QWindow *window)
    : QPlatformBackingStore(window),
      m_needsPosting(false),
      m_scrolled(false)
{
    qCDebug(lcQpaBackingStore) << Q_FUNC_INFO << "w =" << window;

    m_window = window;
}

QQnxRasterBackingStore::~QQnxRasterBackingStore()
{
    qCDebug(lcQpaBackingStore) << Q_FUNC_INFO << "w =" << window();
}

QPaintDevice *QQnxRasterBackingStore::paintDevice()
{
    if (platformWindow() && platformWindow()->hasBuffers())
        return platformWindow()->renderBuffer().image();

    return 0;
}

void QQnxRasterBackingStore::flush(QWindow *window, const QRegion &region, const QPoint &offset)
{
    Q_UNUSED(offset);

    qCDebug(lcQpaBackingStore) << Q_FUNC_INFO << "w =" << this->window();

    // Sometimes this method is called even though there is nothing to be
    // flushed (posted in "screen" parlance), for instance, after an expose
    // event directly follows a geometry change event.
    if (!m_needsPosting)
        return;

    auto *targetWindow = window
        ? static_cast<QQnxRasterWindow *>(window->handle()) : platformWindow();

    if (targetWindow)
        targetWindow->post(region);  // update the display with newly rendered content

    m_needsPosting = false;
    m_scrolled = false;
}

void QQnxRasterBackingStore::resize(const QSize &size, const QRegion &staticContents)
{
    Q_UNUSED(size);
    Q_UNUSED(staticContents);
    qCDebug(lcQpaBackingStore) << Q_FUNC_INFO << "w =" << window() << ", s =" << size;

    // NOTE: defer resizing window buffers until next paint as
    // resize() can be called multiple times before a paint occurs
}

bool QQnxRasterBackingStore::scroll(const QRegion &area, int dx, int dy)
{
    qCDebug(lcQpaBackingStore) << Q_FUNC_INFO << "w =" << window();

    m_needsPosting = true;

    if (!m_scrolled) {
#if defined(QQNX_INCREMENTAL_RASTER_UPDATE)
        platformWindow()->scroll(area, dx, dy, true);
#else
        platformWindow()->scroll(area, dx, dy, false);
        QRegion remainder = QRect(QPoint(0, 0), backingStore()->size());
        remainder -= area.translated(dx, dy);
        platformWindow()->scroll(remainder, 0, 0, true);
#endif
        m_scrolled = true;
        return true;
    }
    return false;
}

void QQnxRasterBackingStore::beginPaint(const QRegion &region)
{
    Q_UNUSED(region);

    qCDebug(lcQpaBackingStore) << Q_FUNC_INFO << "w =" << window();
    m_needsPosting = true;

    platformWindow()->adjustBufferSize();

#if defined(QQNX_INCREMENTAL_RASTER_UPDATE)
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
#else
    if (!m_scrolled)
        platformWindow()->scroll(QRect(QPoint(0, 0), backingStore()->size()), 0, 0, true);
#endif
}

void QQnxRasterBackingStore::endPaint()
{
    qCDebug(lcQpaBackingStore) << Q_FUNC_INFO << "w =" << window();
}

QQnxRasterWindow *QQnxRasterBackingStore::platformWindow() const
{
  Q_ASSERT(m_window->handle());
  return static_cast<QQnxRasterWindow*>(m_window->handle());
}

QT_END_NAMESPACE

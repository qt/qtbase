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

#include "qoffscreencommon.h"
#include "qoffscreenwindow.h"

#include <QtGui/private/qpixmap_raster_p.h>
#include <QtGui/private/qguiapplication_p.h>

#include <qpa/qplatformcursor.h>
#include <qpa/qplatformwindow.h>

QT_BEGIN_NAMESPACE

QPlatformWindow *QOffscreenScreen::windowContainingCursor = 0;

class QOffscreenCursor : public QPlatformCursor
{
public:
    QOffscreenCursor() : m_pos(10, 10) {}

    QPoint pos() const override { return m_pos; }
    void setPos(const QPoint &pos) override
    {
        m_pos = pos;
        const QWindowList wl = QGuiApplication::topLevelWindows();
        QWindow *containing = 0;
        for (QWindow *w : wl) {
            if (w->type() != Qt::Desktop && w->isExposed() && w->geometry().contains(pos)) {
                containing = w;
                break;
            }
        }

        QPoint local = pos;
        if (containing)
            local -= containing->position();

        QWindow *previous = QOffscreenScreen::windowContainingCursor ? QOffscreenScreen::windowContainingCursor->window() : 0;

        if (containing != previous)
            QWindowSystemInterface::handleEnterLeaveEvent(containing, previous, local, pos);

        QWindowSystemInterface::handleMouseEvent(containing, local, pos, QGuiApplication::mouseButtons(), QGuiApplication::keyboardModifiers());

        QOffscreenScreen::windowContainingCursor = containing ? containing->handle() : 0;
    }
#ifndef QT_NO_CURSOR
    void changeCursor(QCursor *windowCursor, QWindow *window) override
    {
        Q_UNUSED(windowCursor);
        Q_UNUSED(window);
    }
#endif
private:
    QPoint m_pos;
};

QOffscreenScreen::QOffscreenScreen()
    : m_geometry(0, 0, 800, 600)
    , m_cursor(new QOffscreenCursor)
{
}

QPixmap QOffscreenScreen::grabWindow(WId id, int x, int y, int width, int height) const
{
    QRect rect(x, y, width, height);

    QOffscreenWindow *window = QOffscreenWindow::windowForWinId(id);
    if (!window || window->window()->type() == Qt::Desktop) {
        const QWindowList wl = QGuiApplication::topLevelWindows();
        QWindow *containing = 0;
        for (QWindow *w : wl) {
            if (w->type() != Qt::Desktop && w->isExposed() && w->geometry().contains(rect)) {
                containing = w;
                break;
            }
        }

        if (!containing)
            return QPixmap();

        id = containing->winId();
        rect = rect.translated(-containing->geometry().topLeft());
    }

    QOffscreenBackingStore *store = QOffscreenBackingStore::backingStoreForWinId(id);
    if (store)
        return store->grabWindow(id, rect);
    return QPixmap();
}

QOffscreenBackingStore::QOffscreenBackingStore(QWindow *window)
    : QPlatformBackingStore(window)
{
}

QOffscreenBackingStore::~QOffscreenBackingStore()
{
    clearHash();
}

QPaintDevice *QOffscreenBackingStore::paintDevice()
{
    return &m_image;
}

void QOffscreenBackingStore::flush(QWindow *window, const QRegion &region, const QPoint &offset)
{
    Q_UNUSED(region);

    if (m_image.size().isEmpty())
        return;

    QSize imageSize = m_image.size();

    QRegion clipped = QRect(0, 0, window->width(), window->height());
    clipped &= QRect(0, 0, imageSize.width(), imageSize.height()).translated(-offset);

    QRect bounds = clipped.boundingRect().translated(offset);

    if (bounds.isNull())
        return;

    WId id = window->winId();

    m_windowAreaHash[id] = bounds;
    m_backingStoreForWinIdHash[id] = this;
}

void QOffscreenBackingStore::resize(const QSize &size, const QRegion &)
{
    QImage::Format format = QGuiApplication::primaryScreen()->handle()->format();
    if (m_image.size() != size)
        m_image = QImage(size, format);
    clearHash();
}

extern void qt_scrollRectInImage(QImage &img, const QRect &rect, const QPoint &offset);

bool QOffscreenBackingStore::scroll(const QRegion &area, int dx, int dy)
{
    if (m_image.isNull())
        return false;

    for (const QRect &rect : area)
        qt_scrollRectInImage(m_image, rect, QPoint(dx, dy));

    return true;
}

QPixmap QOffscreenBackingStore::grabWindow(WId window, const QRect &rect) const
{
    QRect area = m_windowAreaHash.value(window, QRect());
    if (area.isNull())
        return QPixmap();

    QRect adjusted = rect;
    if (adjusted.width() <= 0)
        adjusted.setWidth(area.width());
    if (adjusted.height() <= 0)
        adjusted.setHeight(area.height());

    adjusted = adjusted.translated(area.topLeft()) & area;

    if (adjusted.isEmpty())
        return QPixmap();

    return QPixmap::fromImage(m_image.copy(adjusted));
}

QOffscreenBackingStore *QOffscreenBackingStore::backingStoreForWinId(WId id)
{
    return m_backingStoreForWinIdHash.value(id, 0);
}

void QOffscreenBackingStore::clearHash()
{
    for (auto it = m_windowAreaHash.cbegin(), end = m_windowAreaHash.cend(); it != end; ++it) {
        const auto it2 = qAsConst(m_backingStoreForWinIdHash).find(it.key());
        if (it2.value() == this)
            m_backingStoreForWinIdHash.erase(it2);
    }
    m_windowAreaHash.clear();
}

QHash<WId, QOffscreenBackingStore *> QOffscreenBackingStore::m_backingStoreForWinIdHash;

QT_END_NAMESPACE

// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qoffscreencommon.h"
#include "qoffscreenintegration.h"
#include "qoffscreenwindow.h"


#include <QtGui/QPainter>
#include <QtGui/private/qpixmap_raster_p.h>
#include <QtGui/private/qguiapplication_p.h>

#include <qpa/qplatformcursor.h>
#include <qpa/qplatformwindow.h>

QT_BEGIN_NAMESPACE

QPlatformWindow *QOffscreenScreen::windowContainingCursor = nullptr;


QList<QPlatformScreen *> QOffscreenScreen::virtualSiblings() const
{
    QList<QPlatformScreen *> platformScreens;
    for (auto screen : m_integration->screens()) {
        platformScreens.append(screen);
    }
    return platformScreens;
}

class QOffscreenCursor : public QPlatformCursor
{
public:
    QOffscreenCursor() : m_pos(10, 10) {}

    QPoint pos() const override { return m_pos; }
    void setPos(const QPoint &pos) override
    {
        m_pos = pos;
        const QWindowList wl = QGuiApplication::topLevelWindows();
        QWindow *containing = nullptr;
        for (QWindow *w : wl) {
            if (w->type() != Qt::Desktop && w->isExposed() && w->geometry().contains(pos)) {
                containing = w;
                break;
            }
        }

        QPoint local = pos;
        if (containing)
            local -= containing->position();

        QWindow *previous = QOffscreenScreen::windowContainingCursor ? QOffscreenScreen::windowContainingCursor->window() : nullptr;

        if (containing != previous)
            QWindowSystemInterface::handleEnterLeaveEvent(containing, previous, local, pos);

        QWindowSystemInterface::handleMouseEvent(containing, local, pos, QGuiApplication::mouseButtons(), Qt::NoButton,
                                                 QEvent::MouseMove, QGuiApplication::keyboardModifiers(), Qt::MouseEventSynthesizedByQt);

        QOffscreenScreen::windowContainingCursor = containing ? containing->handle() : nullptr;
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

QOffscreenScreen::QOffscreenScreen(const QOffscreenIntegration *integration)
    : m_geometry(0, 0, 800, 600)
    , m_cursor(new QOffscreenCursor)
    , m_integration(integration)
{
}

QPixmap QOffscreenScreen::grabWindow(WId id, int x, int y, int width, int height) const
{
    QRect rect(x, y, width, height);

    // id == 0 -> grab the screen, so all windows intersecting rect
    if (!id) {
        if (width == -1)
            rect.setWidth(m_geometry.width());
        if (height == -1)
            rect.setHeight(m_geometry.height());
        QPixmap screenImage(rect.size());
        QPainter painter(&screenImage);
        painter.translate(-x, -y);
        const QWindowList wl = QGuiApplication::topLevelWindows();
        for (QWindow *w : wl) {
            if (w->isExposed() && w->geometry().intersects(rect)) {
                QOffscreenBackingStore *store = QOffscreenBackingStore::backingStoreForWinId(w->winId());
                const QImage windowImage = store ? store->toImage() : QImage();
                if (!windowImage.isNull())
                    painter.drawImage(w->position(), windowImage);
            }
        }
        return screenImage;
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
    QImage::Format format = window()->format().hasAlpha()
        ? QImage::Format_ARGB32_Premultiplied
        : QGuiApplication::primaryScreen()->handle()->format();
    if (m_image.size() != size)
        m_image = QImage(size, format);
    clearHash();
}

extern void qt_scrollRectInImage(QImage &img, const QRect &rect, const QPoint &offset);

bool QOffscreenBackingStore::scroll(const QRegion &area, int dx, int dy)
{
    if (m_image.isNull())
        return false;

    const QRect rect = area.boundingRect();
    qt_scrollRectInImage(m_image, rect, QPoint(dx, dy));

    return true;
}

void QOffscreenBackingStore::beginPaint(const QRegion &region)
{
    if (QImage::toPixelFormat(m_image.format()).alphaUsage() == QPixelFormat::UsesAlpha) {
        QPainter p(&m_image);
        p.setCompositionMode(QPainter::CompositionMode_Source);
        const QColor blank = Qt::transparent;
        for (const QRect &r : region)
            p.fillRect(r, blank);
    }
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
    return m_backingStoreForWinIdHash.value(id, nullptr);
}

void QOffscreenBackingStore::clearHash()
{
    for (auto it = m_windowAreaHash.cbegin(), end = m_windowAreaHash.cend(); it != end; ++it) {
        const auto it2 = std::as_const(m_backingStoreForWinIdHash).find(it.key());
        if (it2.value() == this)
            m_backingStoreForWinIdHash.erase(it2);
    }
    m_windowAreaHash.clear();
}

QHash<WId, QOffscreenBackingStore *> QOffscreenBackingStore::m_backingStoreForWinIdHash;

QOffscreenPlatformNativeInterface::QOffscreenPlatformNativeInterface(QOffscreenIntegration *integration)
    : m_integration(integration)
{

}

QOffscreenPlatformNativeInterface::~QOffscreenPlatformNativeInterface() = default;

/*
    Set platform configuration, e.g. screen configuration
*/
void QOffscreenPlatformNativeInterface::setConfiguration(const QJsonObject &configuration, QOffscreenPlatformNativeInterface *iface)
{
    iface->m_integration->setConfiguration(configuration);
}

/*
    Get the current platform configuration
*/
QJsonObject QOffscreenPlatformNativeInterface::configuration(QOffscreenPlatformNativeInterface *iface)
{
    return iface->m_integration->configuration();
}

void *QOffscreenPlatformNativeInterface::nativeResourceForIntegration(const QByteArray &resource)
{
    if (resource == "setConfiguration")
        return reinterpret_cast<void*>(&QOffscreenPlatformNativeInterface::setConfiguration);
    else if (resource == "configuration")
        return reinterpret_cast<void*>(&QOffscreenPlatformNativeInterface::configuration);
    else
        return nullptr;
}

QT_END_NAMESPACE

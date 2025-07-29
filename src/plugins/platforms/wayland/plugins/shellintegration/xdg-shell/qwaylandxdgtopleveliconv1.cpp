// Copyright (C) 2024 David Reondo <kde@david-redondo.de>
// Copyright (C) 2024 David Edmundson <davidedmundson@kde.org>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qwaylandshmbackingstore_p.h"
#include "qwaylandxdgtopleveliconv1_p.h"

#include <QtWaylandClient/private/qwaylanddisplay_p.h>
#include <QtWaylandClient/private/qwaylandshmbackingstore_p.h>

#include <QIcon>
#include <QDir>
#include <QPainter>

QT_BEGIN_NAMESPACE

namespace QtWaylandClient {

class QWaylandXdgToplevelIconV1 : public QtWayland::xdg_toplevel_icon_v1
{
public:
    QWaylandXdgToplevelIconV1(::xdg_toplevel_icon_v1 *object, QWaylandDisplay *display)
        : QtWayland::xdg_toplevel_icon_v1(object), mDisplay(display)
    {
    }

    ~QWaylandXdgToplevelIconV1() override {
        destroy();
    }

    void addPixmap(const QPixmap &pixmap)
    {
        Q_ASSERT(!pixmap.isNull());
        const QSize squareSize = pixmap.size().expandedTo(pixmap.size().transposed());
        auto buffer = std::make_unique<QWaylandShmBuffer>(mDisplay, squareSize, QImage::Format_ARGB32, pixmap.devicePixelRatio());
        QRect targetRect = pixmap.rect();
        targetRect.moveCenter(buffer->image()->rect().center());
        QPainter painter(buffer->image());
        painter.drawPixmap(targetRect, pixmap, pixmap.rect());
        add_buffer(buffer->buffer(), buffer->scale());
        mBuffers.push_back(std::move(buffer));
    }

private:
    QWaylandDisplay *mDisplay;
    std::vector<std::unique_ptr<QWaylandShmBuffer>> mBuffers;
};

QWaylandXdgToplevelIconManagerV1::QWaylandXdgToplevelIconManagerV1(QWaylandDisplay *display,
                                                                   wl_registry *registry,
                                                                   uint32_t id, int version)
    : QtWayland::xdg_toplevel_icon_manager_v1(registry, id, version), mDisplay(display)
{
}

QWaylandXdgToplevelIconManagerV1::~QWaylandXdgToplevelIconManagerV1()
{
    destroy();
}

void QWaylandXdgToplevelIconManagerV1::xdg_toplevel_icon_manager_v1_icon_size(int32_t size)
{
    mPreferredSizes.push_back(size);
}

void QWaylandXdgToplevelIconManagerV1::xdg_toplevel_icon_manager_v1_done() { }

void QWaylandXdgToplevelIconManagerV1::setIcon(const QIcon &icon, xdg_toplevel *window)
{
    if (icon.isNull()) {
        set_icon(window, nullptr);
        return;
    }

    auto toplevelIcon = std::make_unique<QWaylandXdgToplevelIconV1>(create_icon(), mDisplay);

    const QString name = icon.name();
    const bool validName = !name.isEmpty() && !QDir::isAbsolutePath(name);
    if (validName) {
        toplevelIcon->set_name(name);
    }

    QList<QSize> iconSizes = icon.availableSizes();
    // if icon has no default size (an SVG)
    if (iconSizes.isEmpty()) {
        iconSizes.reserve(mPreferredSizes.size());
        for (int size : std::as_const(mPreferredSizes)) {
            iconSizes.append(QSize(size, size));
        }
    }
    // if the compositor hasn't sent a preferred size
    if (iconSizes.isEmpty()) {
        iconSizes.append(QSize(64, 64));
    }

    bool hasPixmaps = false;
    for (const QSize &size : std::as_const(iconSizes)) {
        const QPixmap pixmap = icon.pixmap(size, 1.0);
        if (pixmap.isNull()) {
            qCWarning(lcQpaWayland) << "QWaylandXdgToplevelIconManagerV1: Failed to get pixmap of size" << size << "for icon" << icon;
            continue;
        }

        toplevelIcon->addPixmap(pixmap);
        hasPixmaps = true;
    }

    if (validName || hasPixmaps) {
        set_icon(window, toplevelIcon->object());
    } else {
        set_icon(window, nullptr);
    }
}

} // namespace QtWaylandClient

QT_END_NAMESPACE

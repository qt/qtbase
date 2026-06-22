// Copyright (C) 2016 Robin Burchell <robin.burchell@viroteck.net>
// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include "qwaylandabstractdecoration_p.h"

#include <private/qobject_p.h>
#include "qwaylandwindow_p.h"
#include "qwaylandshellsurface_p.h"
#include "qwaylandinputdevice_p.h"
#include "qwaylandscreen_p.h"

#include <QtGui/QImage>

QT_BEGIN_NAMESPACE

namespace QtWaylandClient {

class QWaylandAbstractDecorationPrivate : public QObjectPrivate
{
    Q_DECLARE_PUBLIC(QWaylandAbstractDecoration)

public:
    QWaylandAbstractDecorationPrivate();
    ~QWaylandAbstractDecorationPrivate() override;

    QWindow *m_window = nullptr;
    QWaylandWindow *m_wayland_window = nullptr;

    bool m_isDirty = true;
    QImage m_decorationContentImage;

    Qt::MouseButtons m_mouseButtons = Qt::NoButton;
};

QWaylandAbstractDecorationPrivate::QWaylandAbstractDecorationPrivate()
    : m_decorationContentImage(nullptr)
{
}

QWaylandAbstractDecorationPrivate::~QWaylandAbstractDecorationPrivate()
{
}

QWaylandAbstractDecoration::QWaylandAbstractDecoration(QObject *parent)
    : QObject(*new QWaylandAbstractDecorationPrivate, parent)
{
}

QWaylandAbstractDecoration::~QWaylandAbstractDecoration()
{
}

// we do this as a setter to get around plugin factory creates not really
// being a great way to pass arguments
void QWaylandAbstractDecoration::setWaylandWindow(QWaylandWindow *window)
{
    Q_D(QWaylandAbstractDecoration);

    // double initialization is probably not great
    Q_ASSERT(!d->m_window && !d->m_wayland_window);

    d->m_window = window->window();
    d->m_wayland_window = window;
}

// Creates regions like this on the outside of a rectangle with inner size \a size
// -----
// |   |
// -----
// I.e. the top and bottom extends into the corners
static QRegion marginsRegion(const QSize &size, const QMargins &margins)
{
    QRegion r;

    r += QRect(0, 0, size.width(), margins.top()); // top
    r += QRect(0, size.height()-margins.bottom(), size.width(), margins.bottom()); //bottom
    r += QRect(0, margins.top(), margins.left(), size.height()); //left
    r += QRect(size.width()-margins.left(), margins.top(), margins.right(), size.height()-margins.top()); // right
    return r;
}

const QImage &QWaylandAbstractDecoration::contentImage()
{
    Q_D(QWaylandAbstractDecoration);
    if (d->m_isDirty) {
        // Update the decoration backingstore

        const qreal bufferScale = waylandWindow()->scale();
        const QSize imageSize = waylandWindow()->surfaceSize() * bufferScale;
        d->m_decorationContentImage = QImage(imageSize, QImage::Format_ARGB32_Premultiplied);
        // Only scale by buffer scale, not QT_SCALE_FACTOR etc.
        d->m_decorationContentImage.setDevicePixelRatio(bufferScale);
        d->m_decorationContentImage.fill(Qt::transparent);
        this->paint(&d->m_decorationContentImage);

        QRegion damage = marginsRegion(waylandWindow()->surfaceSize(), waylandWindow()->frameMargins());
        for (QRect r : damage)
            waylandWindow()->damage(r);

        d->m_isDirty = false;
    }

    return d->m_decorationContentImage;
}

void QWaylandAbstractDecoration::update()
{
    Q_D(QWaylandAbstractDecoration);
    d->m_isDirty = true;
}

void QWaylandAbstractDecoration::setMouseButtons(Qt::MouseButtons mb)
{
    Q_D(QWaylandAbstractDecoration);
    d->m_mouseButtons = mb;
}

void QWaylandAbstractDecoration::startResize(QWaylandInputDevice *inputDevice, Qt::Edges edges, Qt::MouseButtons buttons)
{
    Q_D(QWaylandAbstractDecoration);
    if (isLeftClicked(buttons) && d->m_wayland_window->shellSurface()) {
        d->m_wayland_window->shellSurface()->resize(inputDevice, edges);
        inputDevice->removeMouseButtonFromState(Qt::LeftButton);
    }
}

void QWaylandAbstractDecoration::startMove(QWaylandInputDevice *inputDevice, Qt::MouseButtons buttons)
{
    Q_D(QWaylandAbstractDecoration);
    if (isLeftClicked(buttons) && d->m_wayland_window->shellSurface()) {
        d->m_wayland_window->shellSurface()->move(inputDevice);
        inputDevice->removeMouseButtonFromState(Qt::LeftButton);
    }
}

void QWaylandAbstractDecoration::showWindowMenu(QWaylandInputDevice *inputDevice)
{
    Q_D(QWaylandAbstractDecoration);
    if (auto *s = d->m_wayland_window->shellSurface())
        s->showWindowMenu(inputDevice);
}

bool QWaylandAbstractDecoration::isLeftClicked(Qt::MouseButtons newMouseButtonState)
{
    Q_D(QWaylandAbstractDecoration);
    return !(d->m_mouseButtons & Qt::LeftButton) && (newMouseButtonState & Qt::LeftButton);
}

bool QWaylandAbstractDecoration::isRightClicked(Qt::MouseButtons newMouseButtonState)
{
    Q_D(QWaylandAbstractDecoration);
    return !(d->m_mouseButtons & Qt::RightButton) && (newMouseButtonState & Qt::RightButton);
}

bool QWaylandAbstractDecoration::isLeftReleased(Qt::MouseButtons newMouseButtonState)
{
    Q_D(QWaylandAbstractDecoration);
    return (d->m_mouseButtons & Qt::LeftButton) && !(newMouseButtonState & Qt::LeftButton);
}

bool QWaylandAbstractDecoration::isDirty() const
{
    Q_D(const QWaylandAbstractDecoration);
    return d->m_isDirty;
}

QWindow *QWaylandAbstractDecoration::window() const
{
    Q_D(const QWaylandAbstractDecoration);
    return d->m_window;
}

QWaylandWindow *QWaylandAbstractDecoration::waylandWindow() const
{
    Q_D(const QWaylandAbstractDecoration);
    return d->m_wayland_window;
}

}

QT_END_NAMESPACE

#include "moc_qwaylandabstractdecoration_p.cpp"

// Copyright (C) 2016 The Qt Company Ltd.
// Copyright (C) 2023 David Edmundson <davidedmundson@kde.org>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qwaylandcursor_p.h"

#include "qwaylanddisplay_p.h"
#include "qwaylandinputdevice_p.h"
#include "qwaylandshmbackingstore_p.h"
#include "qwayland-pointer-warp-v1.h"

#include <QtGui/private/qguiapplication_p.h>
#include <qpa/qplatformtheme.h>

#include <QtGui/QImageReader>
#include <QDebug>

#include <wayland-cursor.h>

#include <algorithm>

QT_BEGIN_NAMESPACE

namespace QtWaylandClient {

std::unique_ptr<QWaylandCursorTheme> QWaylandCursorTheme::create(QWaylandShm *shm, int size, const QString &themeName)
{
    QByteArray nameBytes = themeName.toLocal8Bit();
    struct ::wl_cursor_theme *theme = wl_cursor_theme_load(nameBytes.constData(), size, shm->object());

    if (!theme) {
        qCWarning(lcQpaWayland) << "Could not load cursor theme" << themeName << "size" << size;
        return nullptr;
    }

    return std::unique_ptr<QWaylandCursorTheme>{new QWaylandCursorTheme(theme)};
}

QWaylandCursorTheme::~QWaylandCursorTheme()
{
    wl_cursor_theme_destroy(m_theme);
}

wl_cursor *QWaylandCursorTheme::requestCursor(WaylandCursor shape)
{
    if (struct wl_cursor *cursor = m_cursors[shape])
        return cursor;

    static Q_CONSTEXPR struct ShapeAndName {
        WaylandCursor shape;
        const char name[33];
    } cursorNamesMap[] = {
        {ArrowCursor, "left_ptr"},
        {ArrowCursor, "default"},
        {ArrowCursor, "top_left_arrow"},
        {ArrowCursor, "left_arrow"},

        {UpArrowCursor, "up_arrow"},

        {CrossCursor, "cross"},

        {WaitCursor, "wait"},
        {WaitCursor, "watch"},
        {WaitCursor, "0426c94ea35c87780ff01dc239897213"},

        {IBeamCursor, "ibeam"},
        {IBeamCursor, "text"},
        {IBeamCursor, "xterm"},

        {SizeVerCursor, "size_ver"},
        {SizeVerCursor, "ns-resize"},
        {SizeVerCursor, "v_double_arrow"},
        {SizeVerCursor, "00008160000006810000408080010102"},

        {SizeHorCursor, "size_hor"},
        {SizeHorCursor, "ew-resize"},
        {SizeHorCursor, "h_double_arrow"},
        {SizeHorCursor, "028006030e0e7ebffc7f7070c0600140"},

        {SizeBDiagCursor, "size_bdiag"},
        {SizeBDiagCursor, "nesw-resize"},
        {SizeBDiagCursor, "50585d75b494802d0151028115016902"},
        {SizeBDiagCursor, "fcf1c3c7cd4491d801f1e1c78f100000"},

        {SizeFDiagCursor, "size_fdiag"},
        {SizeFDiagCursor, "nwse-resize"},
        {SizeFDiagCursor, "38c5dff7c7b8962045400281044508d2"},
        {SizeFDiagCursor, "c7088f0f3e6c8088236ef8e1e3e70000"},

        {SizeAllCursor, "size_all"},

        {BlankCursor, "blank"},

        {SplitVCursor, "split_v"},
        {SplitVCursor, "row-resize"},
        {SplitVCursor, "sb_v_double_arrow"},
        {SplitVCursor, "2870a09082c103050810ffdffffe0204"},
        {SplitVCursor, "c07385c7190e701020ff7ffffd08103c"},

        {SplitHCursor, "split_h"},
        {SplitHCursor, "col-resize"},
        {SplitHCursor, "sb_h_double_arrow"},
        {SplitHCursor, "043a9f68147c53184671403ffa811cc5"},
        {SplitHCursor, "14fef782d02440884392942c11205230"},

        {PointingHandCursor, "pointing_hand"},
        {PointingHandCursor, "pointer"},
        {PointingHandCursor, "hand1"},
        {PointingHandCursor, "e29285e634086352946a0e7090d73106"},

        {ForbiddenCursor, "forbidden"},
        {ForbiddenCursor, "not-allowed"},
        {ForbiddenCursor, "crossed_circle"},
        {ForbiddenCursor, "circle"},
        {ForbiddenCursor, "03b6e0fcb3499374a867c041f52298f0"},

        {WhatsThisCursor, "whats_this"},
        {WhatsThisCursor, "help"},
        {WhatsThisCursor, "question_arrow"},
        {WhatsThisCursor, "5c6cd98b3f3ebcb1f9c7f1c204630408"},
        {WhatsThisCursor, "d9ce0ab605698f320427677b458ad60b"},

        {BusyCursor, "left_ptr_watch"},
        {BusyCursor, "half-busy"},
        {BusyCursor, "progress"},
        {BusyCursor, "00000000000000020006000e7e9ffc3f"},
        {BusyCursor, "08e8e1c95fe2fc01f976f1e063a24ccd"},

        {OpenHandCursor, "openhand"},
        {OpenHandCursor, "fleur"},
        {OpenHandCursor, "5aca4d189052212118709018842178c0"},
        {OpenHandCursor, "9d800788f1b08800ae810202380a0822"},

        {ClosedHandCursor, "closedhand"},
        {ClosedHandCursor, "grabbing"},
        {ClosedHandCursor, "208530c400c041818281048008011002"},

        {DragCopyCursor, "dnd-copy"},
        {DragCopyCursor, "copy"},

        {DragMoveCursor, "dnd-move"},
        {DragMoveCursor, "move"},

        {DragLinkCursor, "dnd-link"},
        {DragLinkCursor, "link"},

        {ResizeNorthCursor, "n-resize"},
        {ResizeNorthCursor, "top_side"},

        {ResizeSouthCursor, "s-resize"},
        {ResizeSouthCursor, "bottom_side"},

        {ResizeEastCursor, "e-resize"},
        {ResizeEastCursor, "right_side"},

        {ResizeWestCursor, "w-resize"},
        {ResizeWestCursor, "left_side"},

        {ResizeNorthWestCursor, "nw-resize"},
        {ResizeNorthWestCursor, "top_left_corner"},

        {ResizeSouthEastCursor, "se-resize"},
        {ResizeSouthEastCursor, "bottom_right_corner"},

        {ResizeNorthEastCursor, "ne-resize"},
        {ResizeNorthEastCursor, "top_right_corner"},

        {ResizeSouthWestCursor, "sw-resize"},
        {ResizeSouthWestCursor, "bottom_left_corner"},
    };

    const auto byShape = [](ShapeAndName lhs, ShapeAndName rhs) {
        return lhs.shape < rhs.shape;
    };
    Q_ASSERT(std::is_sorted(std::begin(cursorNamesMap), std::end(cursorNamesMap), byShape));
    const auto p = std::equal_range(std::begin(cursorNamesMap), std::end(cursorNamesMap),
                                    ShapeAndName{shape, ""}, byShape);
    for (auto it = p.first; it != p.second; ++it) {
        if (wl_cursor *cursor = wl_cursor_theme_get_cursor(m_theme, it->name)) {
            m_cursors[shape] = cursor;
            return cursor;
        }
    }

    // Fallback to arrow cursor
    if (shape != ArrowCursor)
        return requestCursor(ArrowCursor);

    // Give up
    return nullptr;
}

::wl_cursor *QWaylandCursorTheme::cursor(Qt::CursorShape shape)
{
    struct wl_cursor *waylandCursor = nullptr;

    if (shape < Qt::BitmapCursor) {
        waylandCursor = requestCursor(WaylandCursor(shape));
    } else if (shape == Qt::BitmapCursor) {
        qCWarning(lcQpaWayland) << "cannot create a wl_cursor_image for a CursorShape";
        return nullptr;
    } else {
        //TODO: Custom cursor logic (for resize arrows)
    }

    if (!waylandCursor) {
        qCWarning(lcQpaWayland) << "Could not find cursor for shape" << shape;
        return nullptr;
    }

    return waylandCursor;
}

QWaylandCursorShape::QWaylandCursorShape(::wp_cursor_shape_device_v1 *object)
    : QtWayland::wp_cursor_shape_device_v1(object)
{}

QWaylandCursorShape::~QWaylandCursorShape()
{
    destroy();
}

static QtWayland::wp_cursor_shape_device_v1::shape qtCursorShapeToWaylandShape(Qt::CursorShape cursorShape)
{
    using QtWayland::wp_cursor_shape_device_v1;

    switch (cursorShape) {
    case Qt::BlankCursor:
    case Qt::CustomCursor:
    case Qt::BitmapCursor:
        // these should have been handled separately before using the shape protocol
        Q_ASSERT(false);
        break;
    case Qt::ArrowCursor:
        return wp_cursor_shape_device_v1::shape_default;
    case Qt::SizeVerCursor:
        return wp_cursor_shape_device_v1::shape_ns_resize;
    case Qt::UpArrowCursor:
        return wp_cursor_shape_device_v1::shape_n_resize;
    case Qt::SizeHorCursor:
        return wp_cursor_shape_device_v1::shape_ew_resize;
    case Qt::CrossCursor:
        return wp_cursor_shape_device_v1::shape_crosshair;
    case Qt::SizeBDiagCursor:
        return wp_cursor_shape_device_v1::shape_nesw_resize;
    case Qt::IBeamCursor:
        return wp_cursor_shape_device_v1::shape_text;
    case Qt::SizeFDiagCursor:
        return wp_cursor_shape_device_v1::shape_nwse_resize;
    case Qt::WaitCursor:
        return wp_cursor_shape_device_v1::shape_wait;
    case Qt::SizeAllCursor:
        return wp_cursor_shape_device_v1::shape_all_scroll;
    case Qt::BusyCursor:
        return wp_cursor_shape_device_v1::shape_progress;
    case Qt::SplitVCursor:
        return wp_cursor_shape_device_v1::shape_row_resize;
    case Qt::ForbiddenCursor:
        return wp_cursor_shape_device_v1::shape_not_allowed;
    case Qt::SplitHCursor:
        return wp_cursor_shape_device_v1::shape_col_resize;
    case Qt::PointingHandCursor:
        return wp_cursor_shape_device_v1::shape_pointer;
    case Qt::OpenHandCursor:
        return wp_cursor_shape_device_v1::shape_grab;
    case Qt::WhatsThisCursor:
        return wp_cursor_shape_device_v1::shape_help;
    case Qt::ClosedHandCursor:
        return wp_cursor_shape_device_v1::shape_grabbing;
    case Qt::DragMoveCursor:
        return wp_cursor_shape_device_v1::shape_move;
    case Qt::DragCopyCursor:
        return wp_cursor_shape_device_v1::shape_copy;
    case Qt::DragLinkCursor:
        return wp_cursor_shape_device_v1::shape_alias;
    }
    return wp_cursor_shape_device_v1::shape_default;
}

void QWaylandCursorShape::setShape(uint32_t serial, Qt::CursorShape shape)
{
    set_shape(serial, qtCursorShapeToWaylandShape(shape));
}

QWaylandCursor::QWaylandCursor(QWaylandDisplay *display)
    : mDisplay(display)
{
}

QSharedPointer<QWaylandBuffer> QWaylandCursor::cursorBitmapBuffer(QWaylandDisplay *display, const QCursor *cursor)
{
    Q_ASSERT(cursor->shape() == Qt::BitmapCursor);
    QImage img = !cursor->pixmap().isNull() ? cursor->pixmap().toImage() : cursor->bitmap().toImage();

    // convert to supported format if necessary
    if (!display->shm()->formatSupported(img.format())) {
        if (cursor->mask().isNull()) {
            img.convertTo(QImage::Format_RGB32);
        } else {
            // preserve mask
            img.convertTo(QImage::Format_ARGB32);
            QPixmap pixmap = QPixmap::fromImage(img);
            pixmap.setMask(cursor->mask());
            img = pixmap.toImage();
        }
    }

    QSharedPointer<QWaylandShmBuffer> buffer(new QWaylandShmBuffer(display, img.size(), img.format()));
    memcpy(buffer->image()->bits(), img.bits(), size_t(img.sizeInBytes()));
    return buffer;
}

void QWaylandCursor::changeCursor(QCursor *cursor, QWindow *window)
{
    if (!window)
        return;
    // Create the buffer here so we don't have to create one per input device
    QSharedPointer<QWaylandBuffer> bitmapBuffer;
    if (cursor && cursor->shape() == Qt::BitmapCursor)
        bitmapBuffer = cursorBitmapBuffer(mDisplay, cursor);

    QWaylandWindow *waylandWindow = static_cast<QWaylandWindow*>(window->handle());
    if (!waylandWindow)
        return;

    if (cursor)
        waylandWindow->setStoredCursor(*cursor);
    else
        waylandWindow->resetStoredCursor();

    for (QWaylandInputDevice *device : mDisplay->inputDevices()) {
        if (device->pointer() && device->pointer()->focusWindow() == waylandWindow)
            device->setCursor(cursor, bitmapBuffer, qCeil(waylandWindow->devicePixelRatio()));
    }

    wl_display_flush(mDisplay->wl_display());
}

void QWaylandCursor::pointerEvent(const QMouseEvent &event)
{
    mLastPos = event.globalPosition().toPoint();
}

QPoint QWaylandCursor::pos() const
{
    return mLastPos;
}

void QWaylandCursor::setPos(const QPoint &pos)
{
    if (mDisplay->pointerWarp()) {
        const auto seats = mDisplay->inputDevices();
        for (auto *seat : seats) {
            if (!seat->pointer() || !seat->pointer()->focusWindow()) {
                continue;
            }
            const auto focus = seat->pointer()->focusWindow();
            if (!focus->windowFrameGeometry().contains(pos)) {
                continue;
            }
            mDisplay->pointerWarp()->warp_pointer(focus->surface(), seat->pointer()->object(), wl_fixed_from_double(pos.x() - focus->windowFrameGeometry().x()), wl_fixed_from_double(pos.y() - focus->windowFrameGeometry().y()), seat->pointer()->mEnterSerial);
            return;
        }
    } else {
        qCWarning(lcQpaWayland) << "Setting cursor position requires pointer warp v1 protocol support";
    }
}

void QWaylandCursor::setPosFromEnterEvent(const QPoint &pos)
{
    mLastPos = pos;
}

QSize QWaylandCursor::size() const
{
    if (const QPlatformTheme *theme = QGuiApplicationPrivate::platformTheme())
        return theme->themeHint(QPlatformTheme::MouseCursorSize).toSize();
    return QSize(24, 24);
}

} // namespace QtWaylandClient

QT_END_NAMESPACE

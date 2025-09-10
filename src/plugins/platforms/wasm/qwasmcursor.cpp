// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qwasmcursor.h"
#include "qwasmscreen.h"
#include "qwasmwindow.h"

#include <QtCore/qbuffer.h>
#include <QtCore/qdebug.h>
#include <QtCore/qstring.h>
#include <QtGui/qwindow.h>

#include <emscripten/emscripten.h>
#include <emscripten/bind.h>

QT_BEGIN_NAMESPACE
using namespace emscripten;

namespace {
QByteArray cursorToCss(const QCursor *cursor)
{
    auto shape = cursor->shape();
    switch (shape) {
    case Qt::ArrowCursor:
        return "default";
    case Qt::UpArrowCursor:
        return "n-resize";
    case Qt::CrossCursor:
        return "crosshair";
    case Qt::WaitCursor:
        return "wait";
    case Qt::IBeamCursor:
        return "text";
    case Qt::SizeVerCursor:
        return "ns-resize";
    case Qt::SizeHorCursor:
        return "ew-resize";
    case Qt::SizeBDiagCursor:
        return "nesw-resize";
    case Qt::SizeFDiagCursor:
        return "nwse-resize";
    case Qt::SizeAllCursor:
        return "move";
    case Qt::BlankCursor:
        return "none";
    case Qt::SplitVCursor:
        return "row-resize";
    case Qt::SplitHCursor:
        return "col-resize";
    case Qt::PointingHandCursor:
        return "pointer";
    case Qt::ForbiddenCursor:
        return "not-allowed";
    case Qt::WhatsThisCursor:
        return "help";
    case Qt::BusyCursor:
        return "progress";
    case Qt::OpenHandCursor:
        return "grab";
    case Qt::ClosedHandCursor:
        return "grabbing";
    case Qt::DragCopyCursor:
        return "copy";
    case Qt::DragMoveCursor:
        return "default";
    case Qt::DragLinkCursor:
        return "alias";
    case Qt::BitmapCursor: {
        auto pixmap = cursor->pixmap();
        QByteArray cursorAsPng;
        QBuffer buffer(&cursorAsPng);
        buffer.open(QBuffer::WriteOnly);
        pixmap.save(&buffer, "PNG");
        buffer.close();
        auto cursorAsBase64 = cursorAsPng.toBase64();
        auto hotSpot = cursor->hotSpot();
        auto encodedCursor =
            QString("url(data:image/png;base64,%1) %2 %3, auto")
                .arg(QString::fromUtf8(cursorAsBase64),
                     QString::number(hotSpot.x()),
                     QString::number(hotSpot.y()));
        return encodedCursor.toUtf8();
        }
    default:
        static_assert(Qt::CustomCursor == 25,
                      "New cursor type added, handle it");
        qWarning() << "QWasmCursor: " << shape << " unsupported";
        return "default";
    }
}
} // namespace

void QWasmCursor::changeCursor(QCursor *windowCursor, QWindow *window)
{
    if (!window)
        return;
    if (QWasmWindow *wasmWindow = static_cast<QWasmWindow *>(window->handle()))
        wasmWindow->setWindowCursor(windowCursor ? cursorToCss(windowCursor) : "default");
}

QT_END_NAMESPACE

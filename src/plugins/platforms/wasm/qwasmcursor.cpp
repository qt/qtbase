// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qwasmcursor.h"
#include "qwasmscreen.h"
#include "qwasmwindow.h"

#include <QtCore/qdebug.h>
#include <QtGui/qwindow.h>

#include <emscripten/emscripten.h>
#include <emscripten/bind.h>

QT_BEGIN_NAMESPACE
using namespace emscripten;

namespace {
QByteArray cursorShapeToCss(Qt::CursorShape shape)
{
    QByteArray cursorName;

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
    default:
        static_assert(Qt::BitmapCursor == 24 && Qt::CustomCursor == 25,
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
        wasmWindow->setWindowCursor(windowCursor ? cursorShapeToCss(windowCursor->shape()) : "default");
}

QT_END_NAMESPACE

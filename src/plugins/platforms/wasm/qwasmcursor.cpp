// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qwasmcursor.h"
#include "qwasmscreen.h"
#include "qwasmstring.h"

#include <QtCore/qdebug.h>
#include <QtGui/qwindow.h>

#include <emscripten/emscripten.h>
#include <emscripten/bind.h>

QT_BEGIN_NAMESPACE
using namespace emscripten;

void QWasmCursor::changeCursor(QCursor *windowCursor, QWindow *window)
{
    if (!window)
        return;
    QScreen *screen = window->screen();
    if (!screen)
        return;

    if (windowCursor) {

        // Bitmap and custom cursors are not implemented (will fall back to "auto")
        if (windowCursor->shape() == Qt::BitmapCursor || windowCursor->shape() >= Qt::CustomCursor)
            qWarning() << "QWasmCursor: bitmap and custom cursors are not supported";


        htmlCursorName = cursorShapeToHtml(windowCursor->shape());
    }
    if (htmlCursorName.isEmpty())
        htmlCursorName = "default";

    setWasmCursor(screen, htmlCursorName);
}

QByteArray QWasmCursor::cursorShapeToHtml(Qt::CursorShape shape)
{
    QByteArray cursorName;

    switch (shape) {
    case Qt::ArrowCursor:
        cursorName = "default";
        break;
    case Qt::UpArrowCursor:
        cursorName = "n-resize";
        break;
    case Qt::CrossCursor:
        cursorName = "crosshair";
        break;
    case Qt::WaitCursor:
        cursorName = "wait";
        break;
    case Qt::IBeamCursor:
        cursorName = "text";
        break;
    case Qt::SizeVerCursor:
        cursorName = "ns-resize";
        break;
    case Qt::SizeHorCursor:
        cursorName = "ew-resize";
        break;
    case Qt::SizeBDiagCursor:
        cursorName = "nesw-resize";
        break;
    case Qt::SizeFDiagCursor:
        cursorName = "nwse-resize";
        break;
    case Qt::SizeAllCursor:
        cursorName = "move";
        break;
    case Qt::BlankCursor:
        cursorName = "none";
        break;
    case Qt::SplitVCursor:
        cursorName = "row-resize";
        break;
    case Qt::SplitHCursor:
        cursorName = "col-resize";
        break;
    case Qt::PointingHandCursor:
        cursorName = "pointer";
        break;
    case Qt::ForbiddenCursor:
        cursorName = "not-allowed";
        break;
    case Qt::WhatsThisCursor:
        cursorName = "help";
        break;
    case Qt::BusyCursor:
        cursorName = "progress";
        break;
    case Qt::OpenHandCursor:
        cursorName = "grab";
        break;
    case Qt::ClosedHandCursor:
        cursorName = "grabbing";
        break;
    case Qt::DragCopyCursor:
        cursorName = "copy";
        break;
    case Qt::DragMoveCursor:
        cursorName = "default";
        break;
    case Qt::DragLinkCursor:
        cursorName = "alias";
        break;
    default:
        break;
    }

    return cursorName;
}

void QWasmCursor::setWasmCursor(QScreen *screen, const QByteArray &name)
{
    QWasmScreen::get(screen)->element()["style"].set("cursor", val(name.constData()));
}

void QWasmCursor::setOverrideWasmCursor(const QCursor &windowCursor, QScreen *screen)
{
    QWasmCursor *wCursor = static_cast<QWasmCursor *>(QWasmScreen::get(screen)->cursor());
    wCursor->setWasmCursor(screen, wCursor->cursorShapeToHtml(windowCursor.shape()));
}

void QWasmCursor::clearOverrideWasmCursor(QScreen *screen)
{
    QWasmCursor *wCursor = static_cast<QWasmCursor *>(QWasmScreen::get(screen)->cursor());
    wCursor->setWasmCursor(screen, wCursor->htmlCursorName);
}

QT_END_NAMESPACE

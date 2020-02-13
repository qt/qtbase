/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 or (at your option) any later version
** approved by the KDE Free Qt Foundation. The licenses are as published by
** the Free Software Foundation and appearing in the file LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qwasmcursor.h"
#include "qwasmscreen.h"
#include "qwasmstring.h"

#include <QtCore/qdebug.h>
#include <QtGui/qwindow.h>

#include <emscripten/emscripten.h>
#include <emscripten/bind.h>

using namespace emscripten;

void QWasmCursor::changeCursor(QCursor *windowCursor, QWindow *window)
{
    if (!windowCursor || !window)
        return;
    QScreen *screen = window->screen();
    if (!screen)
        return;

    // Bitmap and custom cursors are not implemented (will fall back to "auto")
    if (windowCursor->shape() == Qt::BitmapCursor || windowCursor->shape() >= Qt::CustomCursor)
        qWarning() << "QWasmCursor: bitmap and custom cursors are not supported";

    QByteArray htmlCursorName = cursorShapeToHtml(windowCursor->shape());

    if (htmlCursorName.isEmpty())
        htmlCursorName = "auto";

    // Set cursor on the canvas
    val canvas = QWasmScreen::get(screen)->canvas();
    val canvasStyle = canvas["style"];
    canvasStyle.set("cursor", val(htmlCursorName.constData()));
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

/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
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

#include "qhtml5cursor.h"

#include <QtCore/qdebug.h>

#include <emscripten/emscripten.h>

void QHtml5Cursor::changeCursor(QCursor *windowCursor, QWindow *window)
{
    qDebug() << "change cursor!" << windowCursor;
    if (windowCursor == nullptr)
        return;

    // FIXME: The HTML5 plugin sets the cursor on the native canvas; when using multiple windows
    // multiple cursors need to be managed taking mouse postion and stacking into account.
    Q_UNUSED(window);

    // Bitmap and custom cursors are not implemented (will fall back to "auto")
    if (windowCursor->shape() == Qt::BitmapCursor || windowCursor->shape() >= Qt::CustomCursor)
        qWarning() << "QHtml5Cursor: bitmap and custom cursors are not supported";

    QByteArray htmlCursorName = cursorShapeToHtml(windowCursor->shape());

    if (htmlCursorName.isEmpty())
        htmlCursorName = "auto";

    // Set cursor on the main canvas
    EM_ASM({
        if (Module['canvas']) {
            Module['canvas'].style['cursor'] = Module['Pointer_stringify']($0);
        }
    }, htmlCursorName.constData());
}

QByteArray QHtml5Cursor::cursorShapeToHtml(Qt::CursorShape shape)
{
    QByteArray cursorName;

    switch (shape) {
    case Qt::ArrowCursor:
        cursorName = "default";
        break;
    case Qt::UpArrowCursor:
        break; // ## missing?
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
        break; // no equivalent?
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
        cursorName = "wait";
        break;
    case Qt::OpenHandCursor:
        break; // no equivalent?
    case Qt::ClosedHandCursor:
        break; // no equivalent?
    case Qt::DragCopyCursor:
        break; // no equivalent?
    case Qt::DragMoveCursor:
        break; // no equivalent?
    case Qt::DragLinkCursor:
        break; // no equivalent?
    default:
        break;
    }

    return cursorName;
}

/****************************************************************************
**
** Copyright (C) 2015 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the Qt for Native Client platform plugin.
**
** $QT_BEGIN_LICENSE:LGPL3$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPLv3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or later as published by the Free
** Software Foundation and appearing in the file LICENSE.GPL included in
** the packaging of this file. Please review the following information to
** ensure the GNU General Public License version 2.0 requirements will be
** met: http://www.gnu.org/licenses/gpl-2.0.html.
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qpeppercursor.h"

#include "qpepperinstance_p.h"

#include <QtGui/QCursor>

#include <ppapi/cpp/mouse_cursor.h>

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(QT_PLATFORM_PEPPER_CURSOR, "qt.platform.pepper.cursor");

static PP_MouseCursor_Type cursorTypeForShape(int cshape)
{
    PP_MouseCursor_Type mouseCursor = PP_MOUSECURSOR_TYPE_POINTER;
    switch (cshape) {
    case Qt::ArrowCursor:
        mouseCursor = PP_MOUSECURSOR_TYPE_POINTER;
        break;
    case Qt::UpArrowCursor:
        break; // ## missing?
    case Qt::CrossCursor:
        mouseCursor = PP_MOUSECURSOR_TYPE_CROSS;
        break;
    case Qt::WaitCursor:
        mouseCursor = PP_MOUSECURSOR_TYPE_WAIT;
        break;
    case Qt::IBeamCursor:
        mouseCursor = PP_MOUSECURSOR_TYPE_IBEAM;
        break;
    case Qt::SizeVerCursor:
        mouseCursor = PP_MOUSECURSOR_TYPE_NORTHSOUTHRESIZE;
        break;
    case Qt::SizeHorCursor:
        mouseCursor = PP_MOUSECURSOR_TYPE_EASTWESTRESIZE;
        break;
    case Qt::SizeBDiagCursor:
        mouseCursor = PP_MOUSECURSOR_TYPE_NORTHEASTSOUTHWESTRESIZE;
        break;
    case Qt::SizeFDiagCursor:
        mouseCursor = PP_MOUSECURSOR_TYPE_NORTHWESTSOUTHEASTRESIZE;
        break;
    case Qt::SizeAllCursor:
        break; // ## missing?
    case Qt::BlankCursor:
        mouseCursor = PP_MOUSECURSOR_TYPE_NONE;
        break;
    case Qt::SplitVCursor:
        break; // ## missing?
    case Qt::SplitHCursor:
        break; // ## missing?
    case Qt::PointingHandCursor:
        mouseCursor = PP_MOUSECURSOR_TYPE_HAND;
        break;
    case Qt::ForbiddenCursor:
        mouseCursor = PP_MOUSECURSOR_TYPE_NOTALLOWED;
        break;
    case Qt::WhatsThisCursor:
        mouseCursor = PP_MOUSECURSOR_TYPE_HELP;
        break;
    case Qt::BusyCursor:
        mouseCursor = PP_MOUSECURSOR_TYPE_WAIT;
        break;
    case Qt::OpenHandCursor:
        mouseCursor = PP_MOUSECURSOR_TYPE_GRAB;
        break;
    case Qt::ClosedHandCursor:
        mouseCursor = PP_MOUSECURSOR_TYPE_GRABBING;
        break;
    case Qt::DragCopyCursor:
        break; // ## missing?
    case Qt::DragMoveCursor:
        break; // ## missing?
    case Qt::DragLinkCursor:
        break; // ## missing?
    default:
        break;
    }
    return mouseCursor;
}

QPepperCursor::QPepperCursor()
    : QPlatformCursor()
{
}

QPepperCursor::~QPepperCursor() {}

void QPepperCursor::changeCursor(QCursor *cursor, QWindow *window)
{
    const int shape = cursor ? cursor->shape() : Qt::CursorShape(Qt::ArrowCursor);
    qCDebug(QT_PLATFORM_PEPPER_CURSOR) << "changeCursor" << shape << window;
    if (shape > Qt::LastCursor)
        qCWarning(QT_PLATFORM_PEPPER_CURSOR) << "Custom cursors are not supported";
    pp::MouseCursor::SetCursor(QPepperInstancePrivate::getPPInstance(), cursorTypeForShape(shape),
                               pp::ImageData(), pp::Point());
}

QT_END_NAMESPACE

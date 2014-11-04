/****************************************************************************
**
** Copyright (C) 2014 Digia Plc
** All rights reserved.
** For any questions to Digia, please use contact form at http://qt.digia.com <http://qt.digia.com/>
**
** This file is part of the Qt Native Client platform plugin.
**
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** If you have questions regarding the use of this file, please use
** contact form at http://qt.digia.com <http://qt.digia.com/>
**
****************************************************************************/

#include "qpeppercursor.h"
#include "qpepperinstance.h"
#include <QtGui/qcursor.h>
#include <ppapi/cpp/mouse_cursor.h>

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(QT_PLATFORM_PEPPER_CURSOR, "qt.platform.pepper.cursor");

static PP_MouseCursor_Type cursorTypeForShape(int cshape)
{
    PP_MouseCursor_Type mouseCursor = PP_MOUSECURSOR_TYPE_POINTER;
    switch(cshape) {
        case Qt::ArrowCursor : mouseCursor = PP_MOUSECURSOR_TYPE_POINTER; break;
        case Qt::UpArrowCursor : break; // ## missing?
        case Qt::CrossCursor : mouseCursor = PP_MOUSECURSOR_TYPE_CROSS; break;
        case Qt::WaitCursor : mouseCursor = PP_MOUSECURSOR_TYPE_WAIT; break;
        case Qt::IBeamCursor : mouseCursor = PP_MOUSECURSOR_TYPE_IBEAM; break;
        case Qt::SizeVerCursor : mouseCursor = PP_MOUSECURSOR_TYPE_NORTHSOUTHRESIZE; break;
        case Qt::SizeHorCursor : mouseCursor = PP_MOUSECURSOR_TYPE_EASTWESTRESIZE; break;
        case Qt::SizeBDiagCursor : mouseCursor = PP_MOUSECURSOR_TYPE_NORTHEASTSOUTHWESTRESIZE; break;
        case Qt::SizeFDiagCursor : mouseCursor = PP_MOUSECURSOR_TYPE_NORTHWESTSOUTHEASTRESIZE; break;
        case Qt::SizeAllCursor : break; // ## missing?
        case Qt::BlankCursor : mouseCursor = PP_MOUSECURSOR_TYPE_NONE; break;
        case Qt::SplitVCursor : break; // ## missing?
        case Qt::SplitHCursor : break; // ## missing?
        case Qt::PointingHandCursor : mouseCursor = PP_MOUSECURSOR_TYPE_HAND; break;
        case Qt::ForbiddenCursor : mouseCursor = PP_MOUSECURSOR_TYPE_NOTALLOWED; break;
        case Qt::WhatsThisCursor : mouseCursor = PP_MOUSECURSOR_TYPE_HELP; break;
        case Qt::BusyCursor : mouseCursor = PP_MOUSECURSOR_TYPE_WAIT; break;
        case Qt::OpenHandCursor : mouseCursor = PP_MOUSECURSOR_TYPE_GRAB; break;
        case Qt::ClosedHandCursor : mouseCursor = PP_MOUSECURSOR_TYPE_GRABBING; break;
        case Qt::DragCopyCursor : break; // ## missing?
        case Qt::DragMoveCursor : break; // ## missing?
        case Qt::DragLinkCursor : break; // ## missing?
        default: break;
    }
    return mouseCursor;
}

QPepperCursor::QPepperCursor()
    : QPlatformCursor()
{

}

QPepperCursor::~QPepperCursor()
{

}

void QPepperCursor::changeCursor(QCursor *cursor, QWindow *window)
{
    const int shape = cursor ? cursor->shape() : Qt::CursorShape(Qt::ArrowCursor);
    qCDebug(QT_PLATFORM_PEPPER_CURSOR) << "changeCursor" << shape << window;
    if (shape > Qt::LastCursor)
        qCWarning(QT_PLATFORM_PEPPER_CURSOR) << "Custom cursors are not supported";
    pp::MouseCursor::SetCursor(QPepperInstance::get(), cursorTypeForShape(shape), pp::ImageData(), pp::Point());
}

QT_END_NAMESPACE

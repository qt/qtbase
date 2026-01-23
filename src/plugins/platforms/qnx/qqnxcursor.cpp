// Copyright (C) 2011 - 2012 Research In Motion
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qqnxglobal.h"
#include "qqnxcursor.h"
#include <errno.h>

#include <QtCore/QDebug>
#include <QWindow>
#include <QCursor>

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(lcQpaQnx, "qt.qpa.qnx");

QQnxCursor::QQnxCursor(screen_context_t context) : m_screenContext(context)
{
}

#if !defined(QT_NO_CURSOR)
static int mapQtCursorToScreenCursor(int cshape)
{
    int cursor_shape;

    switch (cshape) {
    case Qt::ArrowCursor:
        cursor_shape = SCREEN_CURSOR_SHAPE_ARROW;
        break;
    case Qt::CrossCursor:
        cursor_shape = SCREEN_CURSOR_SHAPE_CROSS;
        break;
    case Qt::WaitCursor:
        cursor_shape = SCREEN_CURSOR_SHAPE_WAIT;
        break;
    case Qt::IBeamCursor:
        cursor_shape = SCREEN_CURSOR_SHAPE_IBEAM;
        break;
    case Qt::PointingHandCursor:
        cursor_shape = SCREEN_CURSOR_SHAPE_HAND;
        break;
    case Qt::OpenHandCursor:
        cursor_shape = SCREEN_CURSOR_SHAPE_GRAB;
        break;
    case Qt::ClosedHandCursor:
        cursor_shape = SCREEN_CURSOR_SHAPE_GRABBING;
        break;
    case Qt::DragMoveCursor:
        cursor_shape = SCREEN_CURSOR_SHAPE_MOVE;
        break;
    default:
        cursor_shape = SCREEN_CURSOR_SHAPE_ARROW;
        break;
    }
    return cursor_shape;
}

void QQnxCursor::changeCursor(QCursor *windowCursor, QWindow *window)
{
    qCDebug(lcQpaQnx) << "QQnxCursor::changeCursor() - shape:" << windowCursor->shape()
                      << "window:" << window;
    if (!window || !window->winId())
        return;
    if (windowCursor && windowCursor->shape() != m_currentCShape) {
        m_currentCShape = windowCursor->shape();
        errno = 0;
        int cursorShape = mapQtCursorToScreenCursor(windowCursor->shape());
        screen_window_t screenWindow = reinterpret_cast<screen_window_t>(window->winId());

        if (!m_session) {
            Q_SCREEN_CHECKERROR(screen_create_session_type(&m_session, m_screenContext,
                                                           SCREEN_EVENT_POINTER),
                                "failed to create session type");
        }
        Q_SCREEN_CHECKERROR(screen_set_session_property_pv(m_session, SCREEN_PROPERTY_WINDOW,
                                                           (void**) &screenWindow),
                            "Failed to set window property");
        Q_SCREEN_CHECKERROR(screen_set_session_property_iv(m_session, SCREEN_PROPERTY_CURSOR,
                                                           &cursorShape), "Failed to set cursor shape");
        Q_SCREEN_CHECKERROR(screen_flush_context(m_screenContext, 0),
                            "Failed to flush screen context");
    }
}
#endif

void QQnxCursor::setPos(const QPoint &pos)
{
    qCDebug(lcQpaQnx) << "QQnxCursor::setPos -" << pos;
    m_pos = pos;
}

QPoint QQnxCursor::pos() const
{
    qCDebug(lcQpaQnx) << "QQnxCursor::pos -" << m_pos;
    return m_pos;
}

QT_END_NAMESPACE

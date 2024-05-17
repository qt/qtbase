// Copyright (C) 2011 - 2012 Research In Motion
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qqnxcursor.h"

#include <QtCore/QDebug>

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(lcQpaQnx, "qt.qpa.qnx");

QQnxCursor::QQnxCursor()
{
}

#if !defined(QT_NO_CURSOR)
void QQnxCursor::changeCursor(QCursor *windowCursor, QWindow *window)
{
    Q_UNUSED(windowCursor);
    Q_UNUSED(window);
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

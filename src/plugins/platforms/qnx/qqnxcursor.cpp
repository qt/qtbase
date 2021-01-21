/***************************************************************************
**
** Copyright (C) 2011 - 2012 Research In Motion
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

#include "qqnxcursor.h"

#include <QtCore/QDebug>

#if defined(QQNXCURSOR_DEBUG)
#define qCursorDebug qDebug
#else
#define qCursorDebug QT_NO_QDEBUG_MACRO
#endif

QT_BEGIN_NAMESPACE

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
    qCursorDebug() << "QQnxCursor::setPos -" << pos;
    m_pos = pos;
}

QPoint QQnxCursor::pos() const
{
    qCursorDebug() << "QQnxCursor::pos -" << m_pos;
    return m_pos;
}

QT_END_NAMESPACE

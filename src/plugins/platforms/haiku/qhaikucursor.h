/***************************************************************************
**
** Copyright (C) 2015 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Tobias Koenig <tobias.koenig@kdab.com>
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

#ifndef QHAIKUCURSOR_H
#define QHAIKUCURSOR_H

#include <qpa/qplatformcursor.h>

#include <Cursor.h>

QT_BEGIN_NAMESPACE

class QHaikuCursor : public QPlatformCursor
{
public:
    QHaikuCursor();

#ifndef QT_NO_CURSOR
    void changeCursor(QCursor *windowCursor, QWindow *window) override;
#endif

private:
    QHash<Qt::CursorShape, BCursorID> m_cursorIds;
    QHash<Qt::CursorShape, BCursor*> m_cursors;
};

QT_END_NAMESPACE

#endif

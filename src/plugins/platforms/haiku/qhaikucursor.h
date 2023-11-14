// Copyright (C) 2015 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Tobias Koenig <tobias.koenig@kdab.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial

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

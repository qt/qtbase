// Copyright (C) 2011 - 2012 Research In Motion
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQNXCURSOR_H
#define QQNXCURSOR_H

#include <screen/screen.h>

#include <qpa/qplatformcursor.h>

QT_BEGIN_NAMESPACE

class QQnxCursor : public QPlatformCursor
{
public:
    QQnxCursor(screen_context_t context);
    ~QQnxCursor();

#if !defined(QT_NO_CURSOR)
    void changeCursor(QCursor *windowCursor, QWindow *window) override;
#endif
    void setPos(const QPoint &pos) override;

    QPoint pos() const override;

private:
    QPoint m_pos;
    screen_context_t m_screenContext;
    screen_session_t m_session = 0;
    int m_currentCShape = 0;
    bool m_customCursorEnabled = false;
};

QT_END_NAMESPACE

#endif // QQNXCURSOR_H

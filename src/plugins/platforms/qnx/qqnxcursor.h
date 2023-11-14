// Copyright (C) 2011 - 2012 Research In Motion
// SPDX-License-Identifier: LicenseRef-Qt-Commercial

#ifndef QQNXCURSOR_H
#define QQNXCURSOR_H

#include <qpa/qplatformcursor.h>

QT_BEGIN_NAMESPACE

class QQnxCursor : public QPlatformCursor
{
public:
    QQnxCursor();

#if !defined(QT_NO_CURSOR)
    void changeCursor(QCursor *windowCursor, QWindow *window) override;
#endif
    void setPos(const QPoint &pos) override;

    QPoint pos() const override;

private:
    QPoint m_pos;
};

QT_END_NAMESPACE

#endif // QQNXCURSOR_H

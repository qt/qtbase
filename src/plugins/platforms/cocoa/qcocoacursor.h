// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QWINDOWSCURSOR_H
#define QWINDOWSCURSOR_H

#include <QtCore>
#include <qpa/qplatformcursor.h>

Q_FORWARD_DECLARE_OBJC_CLASS(NSCursor);

QT_BEGIN_NAMESPACE

class QCocoaCursor : public QPlatformCursor
{
public:
    QCocoaCursor();
    ~QCocoaCursor();

    void changeCursor(QCursor *cursor, QWindow *window) override;
    QPoint pos() const override;
    void setPos(const QPoint &position) override;

    QSize size() const override;

private:
    QHash<Qt::CursorShape, NSCursor *> m_cursors;
    NSCursor *convertCursor(QCursor *cursor);
    NSCursor *createCursorData(QCursor * cursor);
    NSCursor *createCursorFromBitmap(const QBitmap &bitmap, const QBitmap &mask, const QPoint hotspot = QPoint());
    NSCursor *createCursorFromPixmap(const QPixmap &pixmap, const QPoint hotspot = QPoint());
};

QT_END_NAMESPACE

#endif // QWINDOWSCURSOR_H

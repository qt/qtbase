/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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

#ifndef QWINDOWSCURSOR_H
#define QWINDOWSCURSOR_H

#include <AppKit/AppKit.h>

#include <QtCore>
#include <qpa/qplatformcursor.h>

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

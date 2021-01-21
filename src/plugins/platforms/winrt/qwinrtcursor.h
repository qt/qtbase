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

#ifndef QWINRTCURSOR_H
#define QWINRTCURSOR_H

#include <qpa/qplatformcursor.h>

QT_BEGIN_NAMESPACE

class QWinRTCursorPrivate;
class QWinRTCursor : public QPlatformCursor
{
public:
    explicit QWinRTCursor();
    ~QWinRTCursor() override = default;
#ifndef QT_NO_CURSOR
    void changeCursor(QCursor * windowCursor, QWindow *window) override;
#endif
    QPoint pos() const override;
    void setPos(const QPoint &pos) override;

private:
    QScopedPointer<QWinRTCursorPrivate> d_ptr;
    Q_DECLARE_PRIVATE(QWinRTCursor)
};

QT_END_NAMESPACE

#endif // QWINRTCURSOR_H

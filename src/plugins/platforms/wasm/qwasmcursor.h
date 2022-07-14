// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QWASMCURSOR_H
#define QWASMCURSOR_H

#include <qpa/qplatformcursor.h>
QT_BEGIN_NAMESPACE

class QWasmCursor : public QPlatformCursor
{
public:
    void changeCursor(QCursor *windowCursor, QWindow *window) override;

    QByteArray cursorShapeToHtml(Qt::CursorShape shape);
    static void setOverrideWasmCursor(const QCursor &windowCursor, QScreen *screen);
    static void clearOverrideWasmCursor(QScreen *screen);
private:
    QByteArray htmlCursorName = "default";
    void setWasmCursor(QScreen *screen, const QByteArray &name);
};

QT_END_NAMESPACE

#endif

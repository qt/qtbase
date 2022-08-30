// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QDIRECTFBCURSOR_H
#define QDIRECTFBCURSOR_H

#include <qpa/qplatformcursor.h>
#include <directfb.h>

#include "qdirectfbconvenience.h"

QT_BEGIN_NAMESPACE

class QDirectFbScreen;
class QDirectFbBlitter;

class QDirectFBCursor : public QPlatformCursor
{
public:
    QDirectFBCursor(QPlatformScreen *screen);
#ifndef QT_NO_CURSOR
    void changeCursor(QCursor *cursor, QWindow *window) override;
#endif

private:
#ifndef QT_NO_CURSOR
    QScopedPointer<QPlatformCursorImage> m_image;
#endif
    QPlatformScreen *m_screen;
};

QT_END_NAMESPACE

#endif // QDIRECTFBCURSOR_H

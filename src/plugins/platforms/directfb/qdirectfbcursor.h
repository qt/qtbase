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
    void changeCursor(QCursor *cursor, QWindow *window);
#endif

private:
#ifndef QT_NO_CURSOR
    QScopedPointer<QPlatformCursorImage> m_image;
#endif
    QPlatformScreen *m_screen;
};

QT_END_NAMESPACE

#endif // QDIRECTFBCURSOR_H

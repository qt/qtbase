
// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QOHOSPLATFORMCURSOR_H
#define QOHOSPLATFORMCURSOR_H

#include <qpa/qplatformcursor.h>

QT_BEGIN_NAMESPACE

class QOhosPlatformCursor : public QPlatformCursor
{
public:
    void changeCursor(QCursor *cursor, QWindow *window) override;
    QPoint pos() const override;
};

QT_END_NAMESPACE

#endif // QOHOSPLATFORMCURSOR_H

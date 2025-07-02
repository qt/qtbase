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
};

QT_END_NAMESPACE

#endif

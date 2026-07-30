// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtCore/QString>

int stringSize()
{
    QString string = QStringLiteral("Hello");
    return string.size();
}

// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtCore/QString>

int main(int, char **)
{
    QString string = QStringLiteral("Hello");
    return string.size() == 5 ? 0 : 1;
}

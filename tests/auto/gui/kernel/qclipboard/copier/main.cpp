// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#include <QtGui/QGuiApplication>
#include <QtGui/QClipboard>
#include <QtCore/QStringList>

int main(int argc, char **argv)
{
    QGuiApplication app(argc, argv);
    QString paste = QStringLiteral("testString.!");
    const QStringList arguments = app.arguments();
    if (arguments.size() > 1)
        paste = arguments.at(1);
#ifndef QT_NO_CLIPBOARD
    QGuiApplication::clipboard()->setText(paste);
#endif
    return 0;
}

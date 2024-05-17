// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QApplication>

#include "codeeditor.h"

int main(int argv, char **args)
{
    QApplication app(argv, args);

    CodeEditor editor;
    editor.setWindowTitle(QObject::tr("Code Editor Example"));
    editor.show();

    return app.exec();
}


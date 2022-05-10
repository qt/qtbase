// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
#include "textedit.h"
#include <QApplication>

int main(int argc, char * argv[])
{
    QApplication app(argc, argv);

    TextEdit textEdit;
    textEdit.show();

    return app.exec();
}

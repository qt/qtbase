// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QApplication>
#include <QTextEdit>

#include "filterobject.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QTextEdit editor;
    FilterObject filter;
    filter.setFilteredObject(&editor);
    editor.show();
    return app.exec();
}

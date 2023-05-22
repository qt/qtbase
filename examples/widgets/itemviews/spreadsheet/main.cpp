// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "spreadsheet.h"

#include <QApplication>
#include <QLayout>

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    SpreadSheet sheet(10, 6);
    sheet.setWindowIcon(QPixmap(":/images/interview.png"));
    sheet.show();
    sheet.layout()->setSizeConstraint(QLayout::SetFixedSize);
    return app.exec();
}

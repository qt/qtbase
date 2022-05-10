// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "object.h"

#include <QtWidgets>
#include <QtPrintSupport/qtprintsupportglobal.h>
#if QT_CONFIG(printdialog)
#include <QPrinter>
#endif

Object::Object(QObject *parent)
    : QObject(parent)
{
}

void Object::print()
{
    int numberOfPages = 10;
    int lastPage = numberOfPages - 1;

//! [0]
    QPrinter printer(QPrinter::HighResolution);
    printer.setOutputFileName("print.ps");
    QPainter painter;
    painter.begin(&printer);

    for (int page = 0; page < numberOfPages; ++page) {

        // Use the painter to draw on the page.

        if (page != lastPage)
            printer.newPage();
    }

    painter.end();
//! [0]
    qApp->quit();
}

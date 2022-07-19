// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef PRINTVIEW_H
#define PRINTVIEW_H

#include <QTableView>
QT_BEGIN_NAMESPACE
class QPrinter;
QT_END_NAMESPACE

class PrintView : public QTableView
{
    Q_OBJECT

public:
    PrintView();

public slots:
    void print(QPrinter *printer);
};

#endif // PRINTVIEW_H



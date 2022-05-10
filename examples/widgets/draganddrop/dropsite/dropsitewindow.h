// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef DROPSITEWINDOW_H
#define DROPSITEWINDOW_H

#include <QWidget>

QT_BEGIN_NAMESPACE
class QDialogButtonBox;
class QLabel;
class QMimeData;
class QPushButton;
class QTableWidget;
QT_END_NAMESPACE
class DropArea;

//! [DropSiteWindow header]
class DropSiteWindow : public QWidget
{
    Q_OBJECT

public:
    DropSiteWindow();

public slots:
    void updateFormatsTable(const QMimeData *mimeData);
    void copy();

private:
    DropArea *dropArea;
    QLabel *abstractLabel;
    QTableWidget *formatsTable;

    QPushButton *clearButton;
    QPushButton *copyButton;
    QPushButton *quitButton;
    QDialogButtonBox *buttonBox;
};
//! [DropSiteWindow header]

#endif // DROPSITEWINDOW_H

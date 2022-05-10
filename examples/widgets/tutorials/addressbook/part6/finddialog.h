// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef FINDDIALOG_H
#define FINDDIALOG_H

#include <QDialog>

QT_BEGIN_NAMESPACE
class QLineEdit;
class QPushButton;
QT_END_NAMESPACE

class FindDialog : public QDialog
{
    Q_OBJECT

public:
    FindDialog(QWidget *parent = nullptr);
    QString getFindText();

public slots:
    void findClicked();

private:
    QPushButton *findButton;
    QLineEdit *lineEdit;
    QString findText;
};

#endif

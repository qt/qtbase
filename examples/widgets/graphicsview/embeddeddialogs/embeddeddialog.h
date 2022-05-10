// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef EMBEDDEDDIALOG_H
#define EMBEDDEDDIALOG_H

#include <QDialog>

QT_BEGIN_NAMESPACE
namespace Ui {
class EmbeddedDialog;
}
QT_END_NAMESPACE

class EmbeddedDialog : public QDialog
{
    Q_OBJECT

public:
    EmbeddedDialog(QWidget *parent = nullptr);
    ~EmbeddedDialog();

private slots:
    void layoutDirectionChanged(int index);
    void spacingChanged(int spacing);
    void fontChanged(const QFont &font);
    void styleChanged(const QString &styleName);

private:
    Ui::EmbeddedDialog *ui;
};

#endif // EMBEDDEDDIALOG_H

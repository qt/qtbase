// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef DIALOG_H
#define DIALOG_H

#include <QDialog>
#include <QQueue>

QT_BEGIN_NAMESPACE
class QComboBox;
class QDialogButtonBox;
class QGridLayout;
class QGroupBox;
class QLabel;
class QPushButton;
QT_END_NAMESPACE

class Dialog : public QDialog
{
    Q_OBJECT

public:
    Dialog(QWidget *parent = nullptr);

private slots:
    void buttonsOrientationChanged(int index);
    void rotateWidgets();
    void help();

private:
    void createRotatableGroupBox();
    void createOptionsGroupBox();
    void createButtonBox();

    QGroupBox *rotatableGroupBox;
    QQueue<QWidget *> rotatableWidgets;

    QGroupBox *optionsGroupBox;
    QLabel *buttonsOrientationLabel;
    QComboBox *buttonsOrientationComboBox;

    QDialogButtonBox *buttonBox;
    QPushButton *closeButton;
    QPushButton *helpButton;
    QPushButton *rotateWidgetsButton;

    QGridLayout *mainLayout;
    QGridLayout *rotatableLayout;
    QGridLayout *optionsLayout;
};

#endif // DIALOG_H

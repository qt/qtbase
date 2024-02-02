/*

* Copyright (C) 2016 The Qt Company Ltd.
* SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

*/

/********************************************************************************
** Form generated from reading UI file 'qtgradientdialog.ui'
**
** Created by: Qt User Interface Compiler version 6.0.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef QTGRADIENTDIALOG_H
#define QTGRADIENTDIALOG_H

#include <QtCore/QVariant>
#include <QtWidgets/QAbstractButton>
#include <QtWidgets/QApplication>
#include <QtWidgets/QDialog>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QVBoxLayout>
#include "qtgradienteditor.h"

QT_BEGIN_NAMESPACE

class Ui_QtGradientDialog
{
public:
    QVBoxLayout *vboxLayout;
    QtGradientEditor *gradientEditor;
    QDialogButtonBox *buttonBox;

    void setupUi(QDialog *QtGradientDialog)
    {
        if (QtGradientDialog->objectName().isEmpty())
            QtGradientDialog->setObjectName("QtGradientDialog");
        QtGradientDialog->resize(178, 81);
        vboxLayout = new QVBoxLayout(QtGradientDialog);
        vboxLayout->setObjectName("vboxLayout");
        gradientEditor = new QtGradientEditor(QtGradientDialog);
        gradientEditor->setObjectName("gradientEditor");
        QSizePolicy sizePolicy(QSizePolicy::Policy::MinimumExpanding, QSizePolicy::Policy::MinimumExpanding);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(gradientEditor->sizePolicy().hasHeightForWidth());
        gradientEditor->setSizePolicy(sizePolicy);

        vboxLayout->addWidget(gradientEditor);

        buttonBox = new QDialogButtonBox(QtGradientDialog);
        buttonBox->setObjectName("buttonBox");
        buttonBox->setOrientation(Qt::Horizontal);
        buttonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::NoButton|QDialogButtonBox::Ok);

        vboxLayout->addWidget(buttonBox);


        retranslateUi(QtGradientDialog);
        QObject::connect(buttonBox, &QDialogButtonBox::accepted, QtGradientDialog, qOverload<>(&QDialog::accept));
        QObject::connect(buttonBox, &QDialogButtonBox::rejected, QtGradientDialog, qOverload<>(&QDialog::reject));

        QMetaObject::connectSlotsByName(QtGradientDialog);
    } // setupUi

    void retranslateUi(QDialog *QtGradientDialog)
    {
        QtGradientDialog->setWindowTitle(QCoreApplication::translate("QtGradientDialog", "Edit Gradient", nullptr));
    } // retranslateUi

};

namespace Ui {
    class QtGradientDialog: public Ui_QtGradientDialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // QTGRADIENTDIALOG_H

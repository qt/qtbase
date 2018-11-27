/*
*********************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the autotests of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
*********************************************************************
*/

/********************************************************************************
** Form generated from reading UI file 'newactiondialog.ui'
**
** Created by: Qt User Interface Compiler version 5.12.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef NEWACTIONDIALOG_H
#define NEWACTIONDIALOG_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QDialog>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QFormLayout>
#include <QtWidgets/QFrame>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QVBoxLayout>
#include "iconselector_p.h"

QT_BEGIN_NAMESPACE

namespace qdesigner_internal {

class Ui_NewActionDialog
{
public:
    QVBoxLayout *verticalLayout;
    QFormLayout *formLayout;
    QLabel *label;
    QLineEdit *editActionText;
    QLabel *label_3;
    QLineEdit *editObjectName;
    QLabel *label_2;
    QHBoxLayout *horizontalLayout;
    qdesigner_internal::IconSelector *iconSelector;
    QSpacerItem *spacerItem;
    QSpacerItem *verticalSpacer;
    QFrame *line;
    QDialogButtonBox *buttonBox;

    void setupUi(QDialog *qdesigner_internal__NewActionDialog)
    {
        if (qdesigner_internal__NewActionDialog->objectName().isEmpty())
            qdesigner_internal__NewActionDialog->setObjectName(QString::fromUtf8("qdesigner_internal__NewActionDialog"));
        qdesigner_internal__NewActionDialog->resize(363, 156);
        verticalLayout = new QVBoxLayout(qdesigner_internal__NewActionDialog);
        verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
        formLayout = new QFormLayout();
        formLayout->setObjectName(QString::fromUtf8("formLayout"));
        label = new QLabel(qdesigner_internal__NewActionDialog);
        label->setObjectName(QString::fromUtf8("label"));

        formLayout->setWidget(0, QFormLayout::LabelRole, label);

        editActionText = new QLineEdit(qdesigner_internal__NewActionDialog);
        editActionText->setObjectName(QString::fromUtf8("editActionText"));
        editActionText->setMinimumSize(QSize(255, 0));

        formLayout->setWidget(0, QFormLayout::FieldRole, editActionText);

        label_3 = new QLabel(qdesigner_internal__NewActionDialog);
        label_3->setObjectName(QString::fromUtf8("label_3"));

        formLayout->setWidget(1, QFormLayout::LabelRole, label_3);

        editObjectName = new QLineEdit(qdesigner_internal__NewActionDialog);
        editObjectName->setObjectName(QString::fromUtf8("editObjectName"));

        formLayout->setWidget(1, QFormLayout::FieldRole, editObjectName);

        label_2 = new QLabel(qdesigner_internal__NewActionDialog);
        label_2->setObjectName(QString::fromUtf8("label_2"));

        formLayout->setWidget(2, QFormLayout::LabelRole, label_2);

        horizontalLayout = new QHBoxLayout();
        horizontalLayout->setObjectName(QString::fromUtf8("horizontalLayout"));
        iconSelector = new qdesigner_internal::IconSelector(qdesigner_internal__NewActionDialog);
        iconSelector->setObjectName(QString::fromUtf8("iconSelector"));

        horizontalLayout->addWidget(iconSelector);

        spacerItem = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout->addItem(spacerItem);


        formLayout->setLayout(2, QFormLayout::FieldRole, horizontalLayout);


        verticalLayout->addLayout(formLayout);

        verticalSpacer = new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding);

        verticalLayout->addItem(verticalSpacer);

        line = new QFrame(qdesigner_internal__NewActionDialog);
        line->setObjectName(QString::fromUtf8("line"));
        line->setFrameShape(QFrame::HLine);
        line->setFrameShadow(QFrame::Sunken);

        verticalLayout->addWidget(line);

        buttonBox = new QDialogButtonBox(qdesigner_internal__NewActionDialog);
        buttonBox->setObjectName(QString::fromUtf8("buttonBox"));
        buttonBox->setOrientation(Qt::Horizontal);
        buttonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);

        verticalLayout->addWidget(buttonBox);

#if QT_CONFIG(shortcut)
        label->setBuddy(editActionText);
        label_3->setBuddy(editObjectName);
        label_2->setBuddy(iconSelector);
#endif // QT_CONFIG(shortcut)
        QWidget::setTabOrder(editActionText, editObjectName);

        retranslateUi(qdesigner_internal__NewActionDialog);
        QObject::connect(buttonBox, SIGNAL(accepted()), qdesigner_internal__NewActionDialog, SLOT(accept()));
        QObject::connect(buttonBox, SIGNAL(rejected()), qdesigner_internal__NewActionDialog, SLOT(reject()));

        QMetaObject::connectSlotsByName(qdesigner_internal__NewActionDialog);
    } // setupUi

    void retranslateUi(QDialog *qdesigner_internal__NewActionDialog)
    {
        qdesigner_internal__NewActionDialog->setWindowTitle(QCoreApplication::translate("qdesigner_internal::NewActionDialog", "New Action...", nullptr));
        label->setText(QCoreApplication::translate("qdesigner_internal::NewActionDialog", "&Text:", nullptr));
        label_3->setText(QCoreApplication::translate("qdesigner_internal::NewActionDialog", "Object &name:", nullptr));
        label_2->setText(QCoreApplication::translate("qdesigner_internal::NewActionDialog", "&Icon:", nullptr));
    } // retranslateUi

};

} // namespace qdesigner_internal

namespace qdesigner_internal {
namespace Ui {
    class NewActionDialog: public Ui_NewActionDialog {};
} // namespace Ui
} // namespace qdesigner_internal

QT_END_NAMESPACE

#endif // NEWACTIONDIALOG_H

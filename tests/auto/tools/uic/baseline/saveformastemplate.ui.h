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
** Form generated from reading UI file 'saveformastemplate.ui'
**
** Created by: Qt User Interface Compiler version 5.0.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef SAVEFORMASTEMPLATE_H
#define SAVEFORMASTEMPLATE_H

#include <QtCore/QVariant>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QDialog>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QFormLayout>
#include <QtWidgets/QFrame>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QVBoxLayout>

QT_BEGIN_NAMESPACE

class Ui_SaveFormAsTemplate
{
public:
    QVBoxLayout *vboxLayout;
    QFormLayout *formLayout;
    QLabel *label;
    QLineEdit *templateNameEdit;
    QLabel *label_2;
    QComboBox *categoryCombo;
    QFrame *horizontalLine;
    QDialogButtonBox *buttonBox;

    void setupUi(QDialog *SaveFormAsTemplate)
    {
        if (SaveFormAsTemplate->objectName().isEmpty())
            SaveFormAsTemplate->setObjectName(QStringLiteral("SaveFormAsTemplate"));
        vboxLayout = new QVBoxLayout(SaveFormAsTemplate);
        vboxLayout->setObjectName(QStringLiteral("vboxLayout"));
        formLayout = new QFormLayout();
        formLayout->setObjectName(QStringLiteral("formLayout"));
        label = new QLabel(SaveFormAsTemplate);
        label->setObjectName(QStringLiteral("label"));
        label->setFrameShape(QFrame::NoFrame);
        label->setFrameShadow(QFrame::Plain);
        label->setTextFormat(Qt::AutoText);

        formLayout->setWidget(0, QFormLayout::LabelRole, label);

        templateNameEdit = new QLineEdit(SaveFormAsTemplate);
        templateNameEdit->setObjectName(QStringLiteral("templateNameEdit"));
        templateNameEdit->setMinimumSize(QSize(222, 0));
        templateNameEdit->setEchoMode(QLineEdit::Normal);

        formLayout->setWidget(0, QFormLayout::FieldRole, templateNameEdit);

        label_2 = new QLabel(SaveFormAsTemplate);
        label_2->setObjectName(QStringLiteral("label_2"));
        label_2->setFrameShape(QFrame::NoFrame);
        label_2->setFrameShadow(QFrame::Plain);
        label_2->setTextFormat(Qt::AutoText);

        formLayout->setWidget(1, QFormLayout::LabelRole, label_2);

        categoryCombo = new QComboBox(SaveFormAsTemplate);
        categoryCombo->setObjectName(QStringLiteral("categoryCombo"));

        formLayout->setWidget(1, QFormLayout::FieldRole, categoryCombo);


        vboxLayout->addLayout(formLayout);

        horizontalLine = new QFrame(SaveFormAsTemplate);
        horizontalLine->setObjectName(QStringLiteral("horizontalLine"));
        horizontalLine->setFrameShape(QFrame::HLine);
        horizontalLine->setFrameShadow(QFrame::Sunken);

        vboxLayout->addWidget(horizontalLine);

        buttonBox = new QDialogButtonBox(SaveFormAsTemplate);
        buttonBox->setObjectName(QStringLiteral("buttonBox"));
        buttonBox->setOrientation(Qt::Horizontal);
        buttonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);

        vboxLayout->addWidget(buttonBox);

#ifndef QT_NO_SHORTCUT
        label->setBuddy(templateNameEdit);
        label_2->setBuddy(categoryCombo);
#endif // QT_NO_SHORTCUT

        retranslateUi(SaveFormAsTemplate);
        QObject::connect(buttonBox, SIGNAL(accepted()), SaveFormAsTemplate, SLOT(accept()));
        QObject::connect(buttonBox, SIGNAL(rejected()), SaveFormAsTemplate, SLOT(reject()));

        QMetaObject::connectSlotsByName(SaveFormAsTemplate);
    } // setupUi

    void retranslateUi(QDialog *SaveFormAsTemplate)
    {
        SaveFormAsTemplate->setWindowTitle(QApplication::translate("SaveFormAsTemplate", "Save Form As Template", Q_NULLPTR));
        label->setText(QApplication::translate("SaveFormAsTemplate", "&Name:", Q_NULLPTR));
        templateNameEdit->setText(QString());
        label_2->setText(QApplication::translate("SaveFormAsTemplate", "&Category:", Q_NULLPTR));
    } // retranslateUi

};

namespace Ui {
    class SaveFormAsTemplate: public Ui_SaveFormAsTemplate {};
} // namespace Ui

QT_END_NAMESPACE

#endif // SAVEFORMASTEMPLATE_H

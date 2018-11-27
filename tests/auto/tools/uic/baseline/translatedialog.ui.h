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
** Form generated from reading UI file 'translatedialog.ui'
**
** Created by: Qt User Interface Compiler version 5.12.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef TRANSLATEDIALOG_H
#define TRANSLATEDIALOG_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QDialog>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QVBoxLayout>

QT_BEGIN_NAMESPACE

class Ui_TranslateDialog
{
public:
    QHBoxLayout *hboxLayout;
    QVBoxLayout *vboxLayout;
    QGridLayout *gridLayout;
    QLineEdit *ledTranslateTo;
    QLabel *findWhat;
    QLabel *translateTo;
    QLineEdit *ledFindWhat;
    QGroupBox *groupBox;
    QVBoxLayout *vboxLayout1;
    QCheckBox *ckMatchCase;
    QCheckBox *ckMarkFinished;
    QSpacerItem *spacerItem;
    QVBoxLayout *vboxLayout2;
    QPushButton *findNxt;
    QPushButton *translate;
    QPushButton *translateAll;
    QPushButton *cancel;
    QSpacerItem *spacerItem1;

    void setupUi(QDialog *TranslateDialog)
    {
        if (TranslateDialog->objectName().isEmpty())
            TranslateDialog->setObjectName(QString::fromUtf8("TranslateDialog"));
        TranslateDialog->resize(407, 145);
        QSizePolicy sizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(TranslateDialog->sizePolicy().hasHeightForWidth());
        TranslateDialog->setSizePolicy(sizePolicy);
        hboxLayout = new QHBoxLayout(TranslateDialog);
        hboxLayout->setSpacing(6);
        hboxLayout->setContentsMargins(11, 11, 11, 11);
        hboxLayout->setObjectName(QString::fromUtf8("hboxLayout"));
        hboxLayout->setContentsMargins(9, 9, 9, 9);
        vboxLayout = new QVBoxLayout();
        vboxLayout->setSpacing(6);
        vboxLayout->setObjectName(QString::fromUtf8("vboxLayout"));
        vboxLayout->setContentsMargins(0, 0, 0, 0);
        gridLayout = new QGridLayout();
        gridLayout->setSpacing(6);
        gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
        gridLayout->setHorizontalSpacing(6);
        gridLayout->setVerticalSpacing(6);
        gridLayout->setContentsMargins(0, 0, 0, 0);
        ledTranslateTo = new QLineEdit(TranslateDialog);
        ledTranslateTo->setObjectName(QString::fromUtf8("ledTranslateTo"));

        gridLayout->addWidget(ledTranslateTo, 1, 1, 1, 1);

        findWhat = new QLabel(TranslateDialog);
        findWhat->setObjectName(QString::fromUtf8("findWhat"));

        gridLayout->addWidget(findWhat, 0, 0, 1, 1);

        translateTo = new QLabel(TranslateDialog);
        translateTo->setObjectName(QString::fromUtf8("translateTo"));

        gridLayout->addWidget(translateTo, 1, 0, 1, 1);

        ledFindWhat = new QLineEdit(TranslateDialog);
        ledFindWhat->setObjectName(QString::fromUtf8("ledFindWhat"));

        gridLayout->addWidget(ledFindWhat, 0, 1, 1, 1);


        vboxLayout->addLayout(gridLayout);

        groupBox = new QGroupBox(TranslateDialog);
        groupBox->setObjectName(QString::fromUtf8("groupBox"));
        vboxLayout1 = new QVBoxLayout(groupBox);
        vboxLayout1->setSpacing(6);
        vboxLayout1->setContentsMargins(11, 11, 11, 11);
        vboxLayout1->setObjectName(QString::fromUtf8("vboxLayout1"));
        ckMatchCase = new QCheckBox(groupBox);
        ckMatchCase->setObjectName(QString::fromUtf8("ckMatchCase"));

        vboxLayout1->addWidget(ckMatchCase);

        ckMarkFinished = new QCheckBox(groupBox);
        ckMarkFinished->setObjectName(QString::fromUtf8("ckMarkFinished"));

        vboxLayout1->addWidget(ckMarkFinished);

        spacerItem = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

        vboxLayout1->addItem(spacerItem);


        vboxLayout->addWidget(groupBox);


        hboxLayout->addLayout(vboxLayout);

        vboxLayout2 = new QVBoxLayout();
        vboxLayout2->setSpacing(6);
        vboxLayout2->setObjectName(QString::fromUtf8("vboxLayout2"));
        vboxLayout2->setContentsMargins(0, 0, 0, 0);
        findNxt = new QPushButton(TranslateDialog);
        findNxt->setObjectName(QString::fromUtf8("findNxt"));
        findNxt->setFlat(false);

        vboxLayout2->addWidget(findNxt);

        translate = new QPushButton(TranslateDialog);
        translate->setObjectName(QString::fromUtf8("translate"));

        vboxLayout2->addWidget(translate);

        translateAll = new QPushButton(TranslateDialog);
        translateAll->setObjectName(QString::fromUtf8("translateAll"));

        vboxLayout2->addWidget(translateAll);

        cancel = new QPushButton(TranslateDialog);
        cancel->setObjectName(QString::fromUtf8("cancel"));

        vboxLayout2->addWidget(cancel);

        spacerItem1 = new QSpacerItem(20, 51, QSizePolicy::Minimum, QSizePolicy::Expanding);

        vboxLayout2->addItem(spacerItem1);


        hboxLayout->addLayout(vboxLayout2);

#if QT_CONFIG(shortcut)
        findWhat->setBuddy(ledFindWhat);
        translateTo->setBuddy(ledTranslateTo);
#endif // QT_CONFIG(shortcut)
        QWidget::setTabOrder(ledFindWhat, ledTranslateTo);
        QWidget::setTabOrder(ledTranslateTo, findNxt);
        QWidget::setTabOrder(findNxt, translate);
        QWidget::setTabOrder(translate, translateAll);
        QWidget::setTabOrder(translateAll, cancel);
        QWidget::setTabOrder(cancel, ckMatchCase);
        QWidget::setTabOrder(ckMatchCase, ckMarkFinished);

        retranslateUi(TranslateDialog);
        QObject::connect(cancel, SIGNAL(clicked()), TranslateDialog, SLOT(reject()));

        findNxt->setDefault(true);


        QMetaObject::connectSlotsByName(TranslateDialog);
    } // setupUi

    void retranslateUi(QDialog *TranslateDialog)
    {
        TranslateDialog->setWindowTitle(QCoreApplication::translate("TranslateDialog", "Qt Linguist", nullptr));
#if QT_CONFIG(whatsthis)
        TranslateDialog->setWhatsThis(QCoreApplication::translate("TranslateDialog", "This window allows you to search for some text in the translation source file.", nullptr));
#endif // QT_CONFIG(whatsthis)
#if QT_CONFIG(whatsthis)
        ledTranslateTo->setWhatsThis(QCoreApplication::translate("TranslateDialog", "Type in the text to search for.", nullptr));
#endif // QT_CONFIG(whatsthis)
        findWhat->setText(QCoreApplication::translate("TranslateDialog", "Find &source text:", nullptr));
        translateTo->setText(QCoreApplication::translate("TranslateDialog", "&Translate to:", nullptr));
#if QT_CONFIG(whatsthis)
        ledFindWhat->setWhatsThis(QCoreApplication::translate("TranslateDialog", "Type in the text to search for.", nullptr));
#endif // QT_CONFIG(whatsthis)
        groupBox->setTitle(QCoreApplication::translate("TranslateDialog", "Search options", nullptr));
#if QT_CONFIG(whatsthis)
        ckMatchCase->setWhatsThis(QCoreApplication::translate("TranslateDialog", "Texts such as 'TeX' and 'tex' are considered as different when checked.", nullptr));
#endif // QT_CONFIG(whatsthis)
        ckMatchCase->setText(QCoreApplication::translate("TranslateDialog", "Match &case", nullptr));
        ckMarkFinished->setText(QCoreApplication::translate("TranslateDialog", "Mark new translation as &finished", nullptr));
#if QT_CONFIG(whatsthis)
        findNxt->setWhatsThis(QCoreApplication::translate("TranslateDialog", "Click here to find the next occurrence of the text you typed in.", nullptr));
#endif // QT_CONFIG(whatsthis)
        findNxt->setText(QCoreApplication::translate("TranslateDialog", "Find Next", nullptr));
        translate->setText(QCoreApplication::translate("TranslateDialog", "Translate", nullptr));
        translateAll->setText(QCoreApplication::translate("TranslateDialog", "Translate All", nullptr));
#if QT_CONFIG(whatsthis)
        cancel->setWhatsThis(QCoreApplication::translate("TranslateDialog", "Click here to close this window.", nullptr));
#endif // QT_CONFIG(whatsthis)
        cancel->setText(QCoreApplication::translate("TranslateDialog", "Cancel", nullptr));
    } // retranslateUi

};

namespace Ui {
    class TranslateDialog: public Ui_TranslateDialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // TRANSLATEDIALOG_H

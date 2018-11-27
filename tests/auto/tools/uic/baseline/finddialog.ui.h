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
** Form generated from reading UI file 'finddialog.ui'
**
** Created by: Qt User Interface Compiler version 5.12.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef FINDDIALOG_H
#define FINDDIALOG_H

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

class Ui_FindDialog
{
public:
    QHBoxLayout *hboxLayout;
    QVBoxLayout *vboxLayout;
    QHBoxLayout *hboxLayout1;
    QLabel *findWhat;
    QLineEdit *led;
    QGroupBox *groupBox;
    QGridLayout *gridLayout;
    QCheckBox *sourceText;
    QCheckBox *translations;
    QCheckBox *matchCase;
    QCheckBox *comments;
    QCheckBox *ignoreAccelerators;
    QVBoxLayout *vboxLayout1;
    QPushButton *findNxt;
    QPushButton *cancel;
    QSpacerItem *spacerItem;

    void setupUi(QDialog *FindDialog)
    {
        if (FindDialog->objectName().isEmpty())
            FindDialog->setObjectName(QString::fromUtf8("FindDialog"));
        FindDialog->resize(414, 170);
        QSizePolicy sizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(FindDialog->sizePolicy().hasHeightForWidth());
        FindDialog->setSizePolicy(sizePolicy);
        hboxLayout = new QHBoxLayout(FindDialog);
        hboxLayout->setSpacing(6);
        hboxLayout->setContentsMargins(11, 11, 11, 11);
        hboxLayout->setObjectName(QString::fromUtf8("hboxLayout"));
        vboxLayout = new QVBoxLayout();
        vboxLayout->setSpacing(6);
        vboxLayout->setContentsMargins(0, 0, 0, 0);
        vboxLayout->setObjectName(QString::fromUtf8("vboxLayout"));
        hboxLayout1 = new QHBoxLayout();
        hboxLayout1->setSpacing(6);
        hboxLayout1->setContentsMargins(0, 0, 0, 0);
        hboxLayout1->setObjectName(QString::fromUtf8("hboxLayout1"));
        findWhat = new QLabel(FindDialog);
        findWhat->setObjectName(QString::fromUtf8("findWhat"));

        hboxLayout1->addWidget(findWhat);

        led = new QLineEdit(FindDialog);
        led->setObjectName(QString::fromUtf8("led"));

        hboxLayout1->addWidget(led);


        vboxLayout->addLayout(hboxLayout1);

        groupBox = new QGroupBox(FindDialog);
        groupBox->setObjectName(QString::fromUtf8("groupBox"));
        gridLayout = new QGridLayout(groupBox);
        gridLayout->setSpacing(6);
        gridLayout->setContentsMargins(9, 9, 9, 9);
        gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
        sourceText = new QCheckBox(groupBox);
        sourceText->setObjectName(QString::fromUtf8("sourceText"));
        sourceText->setChecked(true);

        gridLayout->addWidget(sourceText, 1, 0, 1, 1);

        translations = new QCheckBox(groupBox);
        translations->setObjectName(QString::fromUtf8("translations"));
        translations->setChecked(true);

        gridLayout->addWidget(translations, 2, 0, 1, 1);

        matchCase = new QCheckBox(groupBox);
        matchCase->setObjectName(QString::fromUtf8("matchCase"));

        gridLayout->addWidget(matchCase, 0, 1, 1, 1);

        comments = new QCheckBox(groupBox);
        comments->setObjectName(QString::fromUtf8("comments"));
        comments->setChecked(true);

        gridLayout->addWidget(comments, 0, 0, 1, 1);

        ignoreAccelerators = new QCheckBox(groupBox);
        ignoreAccelerators->setObjectName(QString::fromUtf8("ignoreAccelerators"));
        ignoreAccelerators->setChecked(true);

        gridLayout->addWidget(ignoreAccelerators, 1, 1, 1, 1);


        vboxLayout->addWidget(groupBox);


        hboxLayout->addLayout(vboxLayout);

        vboxLayout1 = new QVBoxLayout();
        vboxLayout1->setSpacing(6);
        vboxLayout1->setContentsMargins(0, 0, 0, 0);
        vboxLayout1->setObjectName(QString::fromUtf8("vboxLayout1"));
        findNxt = new QPushButton(FindDialog);
        findNxt->setObjectName(QString::fromUtf8("findNxt"));
        findNxt->setFlat(false);

        vboxLayout1->addWidget(findNxt);

        cancel = new QPushButton(FindDialog);
        cancel->setObjectName(QString::fromUtf8("cancel"));

        vboxLayout1->addWidget(cancel);

        spacerItem = new QSpacerItem(20, 51, QSizePolicy::Minimum, QSizePolicy::Expanding);

        vboxLayout1->addItem(spacerItem);


        hboxLayout->addLayout(vboxLayout1);

#if QT_CONFIG(shortcut)
        findWhat->setBuddy(led);
#endif // QT_CONFIG(shortcut)
        QWidget::setTabOrder(led, findNxt);
        QWidget::setTabOrder(findNxt, cancel);
        QWidget::setTabOrder(cancel, comments);
        QWidget::setTabOrder(comments, sourceText);
        QWidget::setTabOrder(sourceText, translations);
        QWidget::setTabOrder(translations, matchCase);

        retranslateUi(FindDialog);
        QObject::connect(cancel, SIGNAL(clicked()), FindDialog, SLOT(reject()));

        findNxt->setDefault(true);


        QMetaObject::connectSlotsByName(FindDialog);
    } // setupUi

    void retranslateUi(QDialog *FindDialog)
    {
        FindDialog->setWindowTitle(QCoreApplication::translate("FindDialog", "Find", nullptr));
#if QT_CONFIG(whatsthis)
        FindDialog->setWhatsThis(QCoreApplication::translate("FindDialog", "This window allows you to search for some text in the translation source file.", nullptr));
#endif // QT_CONFIG(whatsthis)
        findWhat->setText(QCoreApplication::translate("FindDialog", "&Find what:", nullptr));
#if QT_CONFIG(whatsthis)
        led->setWhatsThis(QCoreApplication::translate("FindDialog", "Type in the text to search for.", nullptr));
#endif // QT_CONFIG(whatsthis)
        groupBox->setTitle(QCoreApplication::translate("FindDialog", "Options", nullptr));
#if QT_CONFIG(whatsthis)
        sourceText->setWhatsThis(QCoreApplication::translate("FindDialog", "Source texts are searched when checked.", nullptr));
#endif // QT_CONFIG(whatsthis)
        sourceText->setText(QCoreApplication::translate("FindDialog", "&Source texts", nullptr));
#if QT_CONFIG(whatsthis)
        translations->setWhatsThis(QCoreApplication::translate("FindDialog", "Translations are searched when checked.", nullptr));
#endif // QT_CONFIG(whatsthis)
        translations->setText(QCoreApplication::translate("FindDialog", "&Translations", nullptr));
#if QT_CONFIG(whatsthis)
        matchCase->setWhatsThis(QCoreApplication::translate("FindDialog", "Texts such as 'TeX' and 'tex' are considered as different when checked.", nullptr));
#endif // QT_CONFIG(whatsthis)
        matchCase->setText(QCoreApplication::translate("FindDialog", "&Match case", nullptr));
#if QT_CONFIG(whatsthis)
        comments->setWhatsThis(QCoreApplication::translate("FindDialog", "Comments and contexts are searched when checked.", nullptr));
#endif // QT_CONFIG(whatsthis)
        comments->setText(QCoreApplication::translate("FindDialog", "&Comments", nullptr));
        ignoreAccelerators->setText(QCoreApplication::translate("FindDialog", "Ignore &accelerators", nullptr));
#if QT_CONFIG(whatsthis)
        findNxt->setWhatsThis(QCoreApplication::translate("FindDialog", "Click here to find the next occurrence of the text you typed in.", nullptr));
#endif // QT_CONFIG(whatsthis)
        findNxt->setText(QCoreApplication::translate("FindDialog", "Find Next", nullptr));
#if QT_CONFIG(whatsthis)
        cancel->setWhatsThis(QCoreApplication::translate("FindDialog", "Click here to close this window.", nullptr));
#endif // QT_CONFIG(whatsthis)
        cancel->setText(QCoreApplication::translate("FindDialog", "Cancel", nullptr));
    } // retranslateUi

};

namespace Ui {
    class FindDialog: public Ui_FindDialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // FINDDIALOG_H

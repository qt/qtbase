/*
*********************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the autotests of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
*********************************************************************
*/

/********************************************************************************
** Form generated from reading UI file 'translatedialog.ui'
**
** Created by: Qt User Interface Compiler version 5.0.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef TRANSLATEDIALOG_H
#define TRANSLATEDIALOG_H

#include <QtCore/QVariant>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QDialog>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QHeaderView>
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
            TranslateDialog->setObjectName(QStringLiteral("TranslateDialog"));
        TranslateDialog->resize(407, 145);
        QSizePolicy sizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(TranslateDialog->sizePolicy().hasHeightForWidth());
        TranslateDialog->setSizePolicy(sizePolicy);
        hboxLayout = new QHBoxLayout(TranslateDialog);
        hboxLayout->setSpacing(6);
        hboxLayout->setContentsMargins(11, 11, 11, 11);
        hboxLayout->setObjectName(QStringLiteral("hboxLayout"));
        hboxLayout->setContentsMargins(9, 9, 9, 9);
        vboxLayout = new QVBoxLayout();
        vboxLayout->setSpacing(6);
        vboxLayout->setObjectName(QStringLiteral("vboxLayout"));
        vboxLayout->setContentsMargins(0, 0, 0, 0);
        gridLayout = new QGridLayout();
        gridLayout->setSpacing(6);
        gridLayout->setObjectName(QStringLiteral("gridLayout"));
        gridLayout->setHorizontalSpacing(6);
        gridLayout->setVerticalSpacing(6);
        gridLayout->setContentsMargins(0, 0, 0, 0);
        ledTranslateTo = new QLineEdit(TranslateDialog);
        ledTranslateTo->setObjectName(QStringLiteral("ledTranslateTo"));

        gridLayout->addWidget(ledTranslateTo, 1, 1, 1, 1);

        findWhat = new QLabel(TranslateDialog);
        findWhat->setObjectName(QStringLiteral("findWhat"));

        gridLayout->addWidget(findWhat, 0, 0, 1, 1);

        translateTo = new QLabel(TranslateDialog);
        translateTo->setObjectName(QStringLiteral("translateTo"));

        gridLayout->addWidget(translateTo, 1, 0, 1, 1);

        ledFindWhat = new QLineEdit(TranslateDialog);
        ledFindWhat->setObjectName(QStringLiteral("ledFindWhat"));

        gridLayout->addWidget(ledFindWhat, 0, 1, 1, 1);


        vboxLayout->addLayout(gridLayout);

        groupBox = new QGroupBox(TranslateDialog);
        groupBox->setObjectName(QStringLiteral("groupBox"));
        vboxLayout1 = new QVBoxLayout(groupBox);
        vboxLayout1->setSpacing(6);
        vboxLayout1->setContentsMargins(11, 11, 11, 11);
        vboxLayout1->setObjectName(QStringLiteral("vboxLayout1"));
        ckMatchCase = new QCheckBox(groupBox);
        ckMatchCase->setObjectName(QStringLiteral("ckMatchCase"));

        vboxLayout1->addWidget(ckMatchCase);

        ckMarkFinished = new QCheckBox(groupBox);
        ckMarkFinished->setObjectName(QStringLiteral("ckMarkFinished"));

        vboxLayout1->addWidget(ckMarkFinished);

        spacerItem = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

        vboxLayout1->addItem(spacerItem);


        vboxLayout->addWidget(groupBox);


        hboxLayout->addLayout(vboxLayout);

        vboxLayout2 = new QVBoxLayout();
        vboxLayout2->setSpacing(6);
        vboxLayout2->setObjectName(QStringLiteral("vboxLayout2"));
        vboxLayout2->setContentsMargins(0, 0, 0, 0);
        findNxt = new QPushButton(TranslateDialog);
        findNxt->setObjectName(QStringLiteral("findNxt"));
        findNxt->setDefault(true);
        findNxt->setFlat(false);

        vboxLayout2->addWidget(findNxt);

        translate = new QPushButton(TranslateDialog);
        translate->setObjectName(QStringLiteral("translate"));

        vboxLayout2->addWidget(translate);

        translateAll = new QPushButton(TranslateDialog);
        translateAll->setObjectName(QStringLiteral("translateAll"));

        vboxLayout2->addWidget(translateAll);

        cancel = new QPushButton(TranslateDialog);
        cancel->setObjectName(QStringLiteral("cancel"));

        vboxLayout2->addWidget(cancel);

        spacerItem1 = new QSpacerItem(20, 51, QSizePolicy::Minimum, QSizePolicy::Expanding);

        vboxLayout2->addItem(spacerItem1);


        hboxLayout->addLayout(vboxLayout2);

#ifndef QT_NO_SHORTCUT
        findWhat->setBuddy(ledFindWhat);
        translateTo->setBuddy(ledTranslateTo);
#endif // QT_NO_SHORTCUT
        QWidget::setTabOrder(ledFindWhat, ledTranslateTo);
        QWidget::setTabOrder(ledTranslateTo, findNxt);
        QWidget::setTabOrder(findNxt, translate);
        QWidget::setTabOrder(translate, translateAll);
        QWidget::setTabOrder(translateAll, cancel);
        QWidget::setTabOrder(cancel, ckMatchCase);
        QWidget::setTabOrder(ckMatchCase, ckMarkFinished);

        retranslateUi(TranslateDialog);
        QObject::connect(cancel, SIGNAL(clicked()), TranslateDialog, SLOT(reject()));

        QMetaObject::connectSlotsByName(TranslateDialog);
    } // setupUi

    void retranslateUi(QDialog *TranslateDialog)
    {
        TranslateDialog->setWindowTitle(QApplication::translate("TranslateDialog", "Qt Linguist", 0));
#ifndef QT_NO_WHATSTHIS
        TranslateDialog->setWhatsThis(QApplication::translate("TranslateDialog", "This window allows you to search for some text in the translation source file.", 0));
#endif // QT_NO_WHATSTHIS
#ifndef QT_NO_WHATSTHIS
        ledTranslateTo->setWhatsThis(QApplication::translate("TranslateDialog", "Type in the text to search for.", 0));
#endif // QT_NO_WHATSTHIS
        findWhat->setText(QApplication::translate("TranslateDialog", "Find &source text:", 0));
        translateTo->setText(QApplication::translate("TranslateDialog", "&Translate to:", 0));
#ifndef QT_NO_WHATSTHIS
        ledFindWhat->setWhatsThis(QApplication::translate("TranslateDialog", "Type in the text to search for.", 0));
#endif // QT_NO_WHATSTHIS
        groupBox->setTitle(QApplication::translate("TranslateDialog", "Search options", 0));
#ifndef QT_NO_WHATSTHIS
        ckMatchCase->setWhatsThis(QApplication::translate("TranslateDialog", "Texts such as 'TeX' and 'tex' are considered as different when checked.", 0));
#endif // QT_NO_WHATSTHIS
        ckMatchCase->setText(QApplication::translate("TranslateDialog", "Match &case", 0));
        ckMarkFinished->setText(QApplication::translate("TranslateDialog", "Mark new translation as &finished", 0));
#ifndef QT_NO_WHATSTHIS
        findNxt->setWhatsThis(QApplication::translate("TranslateDialog", "Click here to find the next occurrence of the text you typed in.", 0));
#endif // QT_NO_WHATSTHIS
        findNxt->setText(QApplication::translate("TranslateDialog", "Find Next", 0));
        translate->setText(QApplication::translate("TranslateDialog", "Translate", 0));
        translateAll->setText(QApplication::translate("TranslateDialog", "Translate All", 0));
#ifndef QT_NO_WHATSTHIS
        cancel->setWhatsThis(QApplication::translate("TranslateDialog", "Click here to close this window.", 0));
#endif // QT_NO_WHATSTHIS
        cancel->setText(QApplication::translate("TranslateDialog", "Cancel", 0));
    } // retranslateUi

};

namespace Ui {
    class TranslateDialog: public Ui_TranslateDialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // TRANSLATEDIALOG_H

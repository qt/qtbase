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
** Form generated from reading UI file 'batchtranslation.ui'
**
** Created by: Qt User Interface Compiler version 5.0.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef BATCHTRANSLATION_H
#define BATCHTRANSLATION_H

#include <QtCore/QVariant>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QDialog>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QListView>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QVBoxLayout>

QT_BEGIN_NAMESPACE

class Ui_databaseTranslationDialog
{
public:
    QVBoxLayout *vboxLayout;
    QGroupBox *groupBox;
    QVBoxLayout *vboxLayout1;
    QCheckBox *ckOnlyUntranslated;
    QCheckBox *ckMarkFinished;
    QGroupBox *groupBox_2;
    QVBoxLayout *vboxLayout2;
    QHBoxLayout *hboxLayout;
    QListView *phrasebookList;
    QVBoxLayout *vboxLayout3;
    QPushButton *moveUpButton;
    QPushButton *moveDownButton;
    QSpacerItem *spacerItem;
    QLabel *label;
    QHBoxLayout *hboxLayout1;
    QSpacerItem *spacerItem1;
    QPushButton *runButton;
    QPushButton *cancelButton;

    void setupUi(QDialog *databaseTranslationDialog)
    {
        if (databaseTranslationDialog->objectName().isEmpty())
            databaseTranslationDialog->setObjectName(QStringLiteral("databaseTranslationDialog"));
        databaseTranslationDialog->resize(425, 370);
        vboxLayout = new QVBoxLayout(databaseTranslationDialog);
#ifndef Q_OS_MAC
        vboxLayout->setSpacing(6);
#endif
#ifndef Q_OS_MAC
        vboxLayout->setContentsMargins(9, 9, 9, 9);
#endif
        vboxLayout->setObjectName(QStringLiteral("vboxLayout"));
        groupBox = new QGroupBox(databaseTranslationDialog);
        groupBox->setObjectName(QStringLiteral("groupBox"));
        QSizePolicy sizePolicy(static_cast<QSizePolicy::Policy>(5), static_cast<QSizePolicy::Policy>(4));
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(groupBox->sizePolicy().hasHeightForWidth());
        groupBox->setSizePolicy(sizePolicy);
        vboxLayout1 = new QVBoxLayout(groupBox);
#ifndef Q_OS_MAC
        vboxLayout1->setSpacing(6);
#endif
#ifndef Q_OS_MAC
        vboxLayout1->setContentsMargins(9, 9, 9, 9);
#endif
        vboxLayout1->setObjectName(QStringLiteral("vboxLayout1"));
        ckOnlyUntranslated = new QCheckBox(groupBox);
        ckOnlyUntranslated->setObjectName(QStringLiteral("ckOnlyUntranslated"));
        ckOnlyUntranslated->setChecked(true);

        vboxLayout1->addWidget(ckOnlyUntranslated);

        ckMarkFinished = new QCheckBox(groupBox);
        ckMarkFinished->setObjectName(QStringLiteral("ckMarkFinished"));
        ckMarkFinished->setChecked(true);

        vboxLayout1->addWidget(ckMarkFinished);


        vboxLayout->addWidget(groupBox);

        groupBox_2 = new QGroupBox(databaseTranslationDialog);
        groupBox_2->setObjectName(QStringLiteral("groupBox_2"));
        QSizePolicy sizePolicy1(static_cast<QSizePolicy::Policy>(5), static_cast<QSizePolicy::Policy>(1));
        sizePolicy1.setHorizontalStretch(0);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(groupBox_2->sizePolicy().hasHeightForWidth());
        groupBox_2->setSizePolicy(sizePolicy1);
        vboxLayout2 = new QVBoxLayout(groupBox_2);
#ifndef Q_OS_MAC
        vboxLayout2->setSpacing(6);
#endif
#ifndef Q_OS_MAC
        vboxLayout2->setContentsMargins(9, 9, 9, 9);
#endif
        vboxLayout2->setObjectName(QStringLiteral("vboxLayout2"));
        hboxLayout = new QHBoxLayout();
#ifndef Q_OS_MAC
        hboxLayout->setSpacing(6);
#endif
#ifndef Q_OS_MAC
        hboxLayout->setContentsMargins(0, 0, 0, 0);
#endif
        hboxLayout->setObjectName(QStringLiteral("hboxLayout"));
        phrasebookList = new QListView(groupBox_2);
        phrasebookList->setObjectName(QStringLiteral("phrasebookList"));
        phrasebookList->setUniformItemSizes(true);

        hboxLayout->addWidget(phrasebookList);

        vboxLayout3 = new QVBoxLayout();
#ifndef Q_OS_MAC
        vboxLayout3->setSpacing(6);
#endif
        vboxLayout3->setContentsMargins(0, 0, 0, 0);
        vboxLayout3->setObjectName(QStringLiteral("vboxLayout3"));
        moveUpButton = new QPushButton(groupBox_2);
        moveUpButton->setObjectName(QStringLiteral("moveUpButton"));

        vboxLayout3->addWidget(moveUpButton);

        moveDownButton = new QPushButton(groupBox_2);
        moveDownButton->setObjectName(QStringLiteral("moveDownButton"));

        vboxLayout3->addWidget(moveDownButton);

        spacerItem = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

        vboxLayout3->addItem(spacerItem);


        hboxLayout->addLayout(vboxLayout3);


        vboxLayout2->addLayout(hboxLayout);

        label = new QLabel(groupBox_2);
        label->setObjectName(QStringLiteral("label"));
        label->setWordWrap(true);

        vboxLayout2->addWidget(label);


        vboxLayout->addWidget(groupBox_2);

        hboxLayout1 = new QHBoxLayout();
#ifndef Q_OS_MAC
        hboxLayout1->setSpacing(6);
#endif
        hboxLayout1->setContentsMargins(0, 0, 0, 0);
        hboxLayout1->setObjectName(QStringLiteral("hboxLayout1"));
        spacerItem1 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        hboxLayout1->addItem(spacerItem1);

        runButton = new QPushButton(databaseTranslationDialog);
        runButton->setObjectName(QStringLiteral("runButton"));

        hboxLayout1->addWidget(runButton);

        cancelButton = new QPushButton(databaseTranslationDialog);
        cancelButton->setObjectName(QStringLiteral("cancelButton"));

        hboxLayout1->addWidget(cancelButton);


        vboxLayout->addLayout(hboxLayout1);


        retranslateUi(databaseTranslationDialog);

        QMetaObject::connectSlotsByName(databaseTranslationDialog);
    } // setupUi

    void retranslateUi(QDialog *databaseTranslationDialog)
    {
        databaseTranslationDialog->setWindowTitle(QApplication::translate("databaseTranslationDialog", "Qt Linguist - Batch Translation", 0));
        groupBox->setTitle(QApplication::translate("databaseTranslationDialog", "Options", 0));
        ckOnlyUntranslated->setText(QApplication::translate("databaseTranslationDialog", "Only translate entries with no translation", 0));
        ckMarkFinished->setText(QApplication::translate("databaseTranslationDialog", "Set translated entries to finished", 0));
        groupBox_2->setTitle(QApplication::translate("databaseTranslationDialog", "Phrase book preference", 0));
        moveUpButton->setText(QApplication::translate("databaseTranslationDialog", "Move up", 0));
        moveDownButton->setText(QApplication::translate("databaseTranslationDialog", "Move down", 0));
        label->setText(QApplication::translate("databaseTranslationDialog", "The batch translator will search through the selected phrasebooks in the order given above.", 0));
        runButton->setText(QApplication::translate("databaseTranslationDialog", "&Run", 0));
        cancelButton->setText(QApplication::translate("databaseTranslationDialog", "&Cancel", 0));
    } // retranslateUi

};

namespace Ui {
    class databaseTranslationDialog: public Ui_databaseTranslationDialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // BATCHTRANSLATION_H

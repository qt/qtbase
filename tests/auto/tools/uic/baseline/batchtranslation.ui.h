/*

* Copyright (C) 2016 The Qt Company Ltd.
* SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

*/

/********************************************************************************
** Form generated from reading UI file 'batchtranslation.ui'
**
** Created by: Qt User Interface Compiler version 6.0.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef BATCHTRANSLATION_H
#define BATCHTRANSLATION_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QDialog>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHBoxLayout>
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
            databaseTranslationDialog->setObjectName("databaseTranslationDialog");
        databaseTranslationDialog->resize(425, 370);
        vboxLayout = new QVBoxLayout(databaseTranslationDialog);
#ifndef Q_OS_MAC
        vboxLayout->setSpacing(6);
#endif
#ifndef Q_OS_MAC
        vboxLayout->setContentsMargins(9, 9, 9, 9);
#endif
        vboxLayout->setObjectName("vboxLayout");
        groupBox = new QGroupBox(databaseTranslationDialog);
        groupBox->setObjectName("groupBox");
        QSizePolicy sizePolicy(QSizePolicy::Policy::Preferred, QSizePolicy::Policy::Maximum);
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
        vboxLayout1->setObjectName("vboxLayout1");
        ckOnlyUntranslated = new QCheckBox(groupBox);
        ckOnlyUntranslated->setObjectName("ckOnlyUntranslated");
        ckOnlyUntranslated->setChecked(true);

        vboxLayout1->addWidget(ckOnlyUntranslated);

        ckMarkFinished = new QCheckBox(groupBox);
        ckMarkFinished->setObjectName("ckMarkFinished");
        ckMarkFinished->setChecked(true);

        vboxLayout1->addWidget(ckMarkFinished);


        vboxLayout->addWidget(groupBox);

        groupBox_2 = new QGroupBox(databaseTranslationDialog);
        groupBox_2->setObjectName("groupBox_2");
        QSizePolicy sizePolicy1(QSizePolicy::Policy::Preferred, QSizePolicy::Policy::Minimum);
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
        vboxLayout2->setObjectName("vboxLayout2");
        hboxLayout = new QHBoxLayout();
#ifndef Q_OS_MAC
        hboxLayout->setSpacing(6);
#endif
#ifndef Q_OS_MAC
        hboxLayout->setContentsMargins(0, 0, 0, 0);
#endif
        hboxLayout->setObjectName("hboxLayout");
        phrasebookList = new QListView(groupBox_2);
        phrasebookList->setObjectName("phrasebookList");
        phrasebookList->setUniformItemSizes(true);

        hboxLayout->addWidget(phrasebookList);

        vboxLayout3 = new QVBoxLayout();
#ifndef Q_OS_MAC
        vboxLayout3->setSpacing(6);
#endif
        vboxLayout3->setContentsMargins(0, 0, 0, 0);
        vboxLayout3->setObjectName("vboxLayout3");
        moveUpButton = new QPushButton(groupBox_2);
        moveUpButton->setObjectName("moveUpButton");

        vboxLayout3->addWidget(moveUpButton);

        moveDownButton = new QPushButton(groupBox_2);
        moveDownButton->setObjectName("moveDownButton");

        vboxLayout3->addWidget(moveDownButton);

        spacerItem = new QSpacerItem(20, 40, QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Expanding);

        vboxLayout3->addItem(spacerItem);


        hboxLayout->addLayout(vboxLayout3);


        vboxLayout2->addLayout(hboxLayout);

        label = new QLabel(groupBox_2);
        label->setObjectName("label");
        label->setWordWrap(true);

        vboxLayout2->addWidget(label);


        vboxLayout->addWidget(groupBox_2);

        hboxLayout1 = new QHBoxLayout();
#ifndef Q_OS_MAC
        hboxLayout1->setSpacing(6);
#endif
        hboxLayout1->setContentsMargins(0, 0, 0, 0);
        hboxLayout1->setObjectName("hboxLayout1");
        spacerItem1 = new QSpacerItem(40, 20, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        hboxLayout1->addItem(spacerItem1);

        runButton = new QPushButton(databaseTranslationDialog);
        runButton->setObjectName("runButton");

        hboxLayout1->addWidget(runButton);

        cancelButton = new QPushButton(databaseTranslationDialog);
        cancelButton->setObjectName("cancelButton");

        hboxLayout1->addWidget(cancelButton);


        vboxLayout->addLayout(hboxLayout1);


        retranslateUi(databaseTranslationDialog);

        QMetaObject::connectSlotsByName(databaseTranslationDialog);
    } // setupUi

    void retranslateUi(QDialog *databaseTranslationDialog)
    {
        databaseTranslationDialog->setWindowTitle(QCoreApplication::translate("databaseTranslationDialog", "Qt Linguist - Batch Translation", nullptr));
        groupBox->setTitle(QCoreApplication::translate("databaseTranslationDialog", "Options", nullptr));
        ckOnlyUntranslated->setText(QCoreApplication::translate("databaseTranslationDialog", "Only translate entries with no translation", nullptr));
        ckMarkFinished->setText(QCoreApplication::translate("databaseTranslationDialog", "Set translated entries to finished", nullptr));
        groupBox_2->setTitle(QCoreApplication::translate("databaseTranslationDialog", "Phrase book preference", nullptr));
        moveUpButton->setText(QCoreApplication::translate("databaseTranslationDialog", "Move up", nullptr));
        moveDownButton->setText(QCoreApplication::translate("databaseTranslationDialog", "Move down", nullptr));
        label->setText(QCoreApplication::translate("databaseTranslationDialog", "The batch translator will search through the selected phrasebooks in the order given above.", nullptr));
        runButton->setText(QCoreApplication::translate("databaseTranslationDialog", "&Run", nullptr));
        cancelButton->setText(QCoreApplication::translate("databaseTranslationDialog", "&Cancel", nullptr));
    } // retranslateUi

};

namespace Ui {
    class databaseTranslationDialog: public Ui_databaseTranslationDialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // BATCHTRANSLATION_H

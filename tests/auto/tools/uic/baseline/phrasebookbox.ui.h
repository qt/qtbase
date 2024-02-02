/*

* Copyright (C) 2016 The Qt Company Ltd.
* SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

*/

/********************************************************************************
** Form generated from reading UI file 'phrasebookbox.ui'
**
** Created by: Qt User Interface Compiler version 6.0.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef PHRASEBOOKBOX_H
#define PHRASEBOOKBOX_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QDialog>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QTreeView>
#include <QtWidgets/QVBoxLayout>

QT_BEGIN_NAMESPACE

class Ui_PhraseBookBox
{
public:
    QHBoxLayout *unnamed;
    QVBoxLayout *inputsLayout;
    QGridLayout *gridLayout;
    QLabel *target;
    QLineEdit *targetLed;
    QLabel *source;
    QLineEdit *definitionLed;
    QLineEdit *sourceLed;
    QLabel *definition;
    QTreeView *phraseList;
    QVBoxLayout *buttonLayout;
    QPushButton *newBut;
    QPushButton *removeBut;
    QPushButton *saveBut;
    QPushButton *closeBut;
    QSpacerItem *spacer1;

    void setupUi(QDialog *PhraseBookBox)
    {
        if (PhraseBookBox->objectName().isEmpty())
            PhraseBookBox->setObjectName("PhraseBookBox");
        PhraseBookBox->resize(596, 454);
        unnamed = new QHBoxLayout(PhraseBookBox);
        unnamed->setSpacing(6);
        unnamed->setContentsMargins(11, 11, 11, 11);
        unnamed->setObjectName("unnamed");
        inputsLayout = new QVBoxLayout();
        inputsLayout->setSpacing(6);
        inputsLayout->setObjectName("inputsLayout");
        gridLayout = new QGridLayout();
        gridLayout->setSpacing(6);
        gridLayout->setObjectName("gridLayout");
        target = new QLabel(PhraseBookBox);
        target->setObjectName("target");

        gridLayout->addWidget(target, 1, 0, 1, 1);

        targetLed = new QLineEdit(PhraseBookBox);
        targetLed->setObjectName("targetLed");

        gridLayout->addWidget(targetLed, 1, 1, 1, 1);

        source = new QLabel(PhraseBookBox);
        source->setObjectName("source");

        gridLayout->addWidget(source, 0, 0, 1, 1);

        definitionLed = new QLineEdit(PhraseBookBox);
        definitionLed->setObjectName("definitionLed");

        gridLayout->addWidget(definitionLed, 2, 1, 1, 1);

        sourceLed = new QLineEdit(PhraseBookBox);
        sourceLed->setObjectName("sourceLed");

        gridLayout->addWidget(sourceLed, 0, 1, 1, 1);

        definition = new QLabel(PhraseBookBox);
        definition->setObjectName("definition");

        gridLayout->addWidget(definition, 2, 0, 1, 1);


        inputsLayout->addLayout(gridLayout);

        phraseList = new QTreeView(PhraseBookBox);
        phraseList->setObjectName("phraseList");
        phraseList->setRootIsDecorated(false);
        phraseList->setUniformRowHeights(true);
        phraseList->setItemsExpandable(false);
        phraseList->setSortingEnabled(true);
        phraseList->setExpandsOnDoubleClick(false);

        inputsLayout->addWidget(phraseList);


        unnamed->addLayout(inputsLayout);

        buttonLayout = new QVBoxLayout();
        buttonLayout->setSpacing(6);
        buttonLayout->setObjectName("buttonLayout");
        newBut = new QPushButton(PhraseBookBox);
        newBut->setObjectName("newBut");

        buttonLayout->addWidget(newBut);

        removeBut = new QPushButton(PhraseBookBox);
        removeBut->setObjectName("removeBut");

        buttonLayout->addWidget(removeBut);

        saveBut = new QPushButton(PhraseBookBox);
        saveBut->setObjectName("saveBut");

        buttonLayout->addWidget(saveBut);

        closeBut = new QPushButton(PhraseBookBox);
        closeBut->setObjectName("closeBut");

        buttonLayout->addWidget(closeBut);

        spacer1 = new QSpacerItem(20, 51, QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Expanding);

        buttonLayout->addItem(spacer1);


        unnamed->addLayout(buttonLayout);

#if QT_CONFIG(shortcut)
        target->setBuddy(targetLed);
        source->setBuddy(sourceLed);
        definition->setBuddy(definitionLed);
#endif // QT_CONFIG(shortcut)
        QWidget::setTabOrder(sourceLed, targetLed);
        QWidget::setTabOrder(targetLed, definitionLed);
        QWidget::setTabOrder(definitionLed, newBut);
        QWidget::setTabOrder(newBut, removeBut);
        QWidget::setTabOrder(removeBut, saveBut);
        QWidget::setTabOrder(saveBut, closeBut);

        retranslateUi(PhraseBookBox);

        QMetaObject::connectSlotsByName(PhraseBookBox);
    } // setupUi

    void retranslateUi(QDialog *PhraseBookBox)
    {
        PhraseBookBox->setWindowTitle(QCoreApplication::translate("PhraseBookBox", "Edit Phrase Book", nullptr));
#if QT_CONFIG(whatsthis)
        PhraseBookBox->setWhatsThis(QCoreApplication::translate("PhraseBookBox", "This window allows you to add, modify, or delete phrases in a phrase book.", nullptr));
#endif // QT_CONFIG(whatsthis)
        target->setText(QCoreApplication::translate("PhraseBookBox", "&Translation:", nullptr));
#if QT_CONFIG(whatsthis)
        targetLed->setWhatsThis(QCoreApplication::translate("PhraseBookBox", "This is the phrase in the target language corresponding to the source phrase.", nullptr));
#endif // QT_CONFIG(whatsthis)
        source->setText(QCoreApplication::translate("PhraseBookBox", "S&ource phrase:", nullptr));
#if QT_CONFIG(whatsthis)
        definitionLed->setWhatsThis(QCoreApplication::translate("PhraseBookBox", "This is a definition for the source phrase.", nullptr));
#endif // QT_CONFIG(whatsthis)
#if QT_CONFIG(whatsthis)
        sourceLed->setWhatsThis(QCoreApplication::translate("PhraseBookBox", "This is the phrase in the source language.", nullptr));
#endif // QT_CONFIG(whatsthis)
        definition->setText(QCoreApplication::translate("PhraseBookBox", "&Definition:", nullptr));
#if QT_CONFIG(whatsthis)
        newBut->setWhatsThis(QCoreApplication::translate("PhraseBookBox", "Click here to add the phrase to the phrase book.", nullptr));
#endif // QT_CONFIG(whatsthis)
        newBut->setText(QCoreApplication::translate("PhraseBookBox", "&New Phrase", nullptr));
#if QT_CONFIG(whatsthis)
        removeBut->setWhatsThis(QCoreApplication::translate("PhraseBookBox", "Click here to remove the phrase from the phrase book.", nullptr));
#endif // QT_CONFIG(whatsthis)
        removeBut->setText(QCoreApplication::translate("PhraseBookBox", "&Remove Phrase", nullptr));
#if QT_CONFIG(whatsthis)
        saveBut->setWhatsThis(QCoreApplication::translate("PhraseBookBox", "Click here to save the changes made.", nullptr));
#endif // QT_CONFIG(whatsthis)
        saveBut->setText(QCoreApplication::translate("PhraseBookBox", "&Save", nullptr));
#if QT_CONFIG(whatsthis)
        closeBut->setWhatsThis(QCoreApplication::translate("PhraseBookBox", "Click here to close this window.", nullptr));
#endif // QT_CONFIG(whatsthis)
        closeBut->setText(QCoreApplication::translate("PhraseBookBox", "Close", nullptr));
    } // retranslateUi

};

namespace Ui {
    class PhraseBookBox: public Ui_PhraseBookBox {};
} // namespace Ui

QT_END_NAMESPACE

#endif // PHRASEBOOKBOX_H

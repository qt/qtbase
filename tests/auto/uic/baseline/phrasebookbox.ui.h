/*
*********************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the autotests of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
*********************************************************************
*/

/********************************************************************************
** Form generated from reading UI file 'phrasebookbox.ui'
**
** Created: Fri Sep 4 10:17:14 2009
**      by: Qt User Interface Compiler version 4.6.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef PHRASEBOOKBOX_H
#define PHRASEBOOKBOX_H

#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QDialog>
#include <QtGui/QGridLayout>
#include <QtGui/QHBoxLayout>
#include <QtGui/QHeaderView>
#include <QtGui/QLabel>
#include <QtGui/QLineEdit>
#include <QtGui/QPushButton>
#include <QtGui/QSpacerItem>
#include <QtGui/QTreeView>
#include <QtGui/QVBoxLayout>

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
            PhraseBookBox->setObjectName(QString::fromUtf8("PhraseBookBox"));
        PhraseBookBox->resize(596, 454);
        unnamed = new QHBoxLayout(PhraseBookBox);
        unnamed->setSpacing(6);
        unnamed->setContentsMargins(11, 11, 11, 11);
        unnamed->setObjectName(QString::fromUtf8("unnamed"));
        inputsLayout = new QVBoxLayout();
        inputsLayout->setSpacing(6);
        inputsLayout->setObjectName(QString::fromUtf8("inputsLayout"));
        gridLayout = new QGridLayout();
        gridLayout->setSpacing(6);
        gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
        target = new QLabel(PhraseBookBox);
        target->setObjectName(QString::fromUtf8("target"));

        gridLayout->addWidget(target, 1, 0, 1, 1);

        targetLed = new QLineEdit(PhraseBookBox);
        targetLed->setObjectName(QString::fromUtf8("targetLed"));

        gridLayout->addWidget(targetLed, 1, 1, 1, 1);

        source = new QLabel(PhraseBookBox);
        source->setObjectName(QString::fromUtf8("source"));

        gridLayout->addWidget(source, 0, 0, 1, 1);

        definitionLed = new QLineEdit(PhraseBookBox);
        definitionLed->setObjectName(QString::fromUtf8("definitionLed"));

        gridLayout->addWidget(definitionLed, 2, 1, 1, 1);

        sourceLed = new QLineEdit(PhraseBookBox);
        sourceLed->setObjectName(QString::fromUtf8("sourceLed"));

        gridLayout->addWidget(sourceLed, 0, 1, 1, 1);

        definition = new QLabel(PhraseBookBox);
        definition->setObjectName(QString::fromUtf8("definition"));

        gridLayout->addWidget(definition, 2, 0, 1, 1);


        inputsLayout->addLayout(gridLayout);

        phraseList = new QTreeView(PhraseBookBox);
        phraseList->setObjectName(QString::fromUtf8("phraseList"));
        phraseList->setRootIsDecorated(false);
        phraseList->setUniformRowHeights(true);
        phraseList->setItemsExpandable(false);
        phraseList->setSortingEnabled(true);
        phraseList->setExpandsOnDoubleClick(false);

        inputsLayout->addWidget(phraseList);


        unnamed->addLayout(inputsLayout);

        buttonLayout = new QVBoxLayout();
        buttonLayout->setSpacing(6);
        buttonLayout->setObjectName(QString::fromUtf8("buttonLayout"));
        newBut = new QPushButton(PhraseBookBox);
        newBut->setObjectName(QString::fromUtf8("newBut"));

        buttonLayout->addWidget(newBut);

        removeBut = new QPushButton(PhraseBookBox);
        removeBut->setObjectName(QString::fromUtf8("removeBut"));

        buttonLayout->addWidget(removeBut);

        saveBut = new QPushButton(PhraseBookBox);
        saveBut->setObjectName(QString::fromUtf8("saveBut"));

        buttonLayout->addWidget(saveBut);

        closeBut = new QPushButton(PhraseBookBox);
        closeBut->setObjectName(QString::fromUtf8("closeBut"));

        buttonLayout->addWidget(closeBut);

        spacer1 = new QSpacerItem(20, 51, QSizePolicy::Minimum, QSizePolicy::Expanding);

        buttonLayout->addItem(spacer1);


        unnamed->addLayout(buttonLayout);

#ifndef QT_NO_SHORTCUT
        target->setBuddy(targetLed);
        source->setBuddy(sourceLed);
        definition->setBuddy(definitionLed);
#endif // QT_NO_SHORTCUT
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
        PhraseBookBox->setWindowTitle(QApplication::translate("PhraseBookBox", "Edit Phrase Book", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_WHATSTHIS
        PhraseBookBox->setWhatsThis(QApplication::translate("PhraseBookBox", "This window allows you to add, modify, or delete phrases in a phrase book.", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_WHATSTHIS
        target->setText(QApplication::translate("PhraseBookBox", "&Translation:", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_WHATSTHIS
        targetLed->setWhatsThis(QApplication::translate("PhraseBookBox", "This is the phrase in the target language corresponding to the source phrase.", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_WHATSTHIS
        source->setText(QApplication::translate("PhraseBookBox", "S&ource phrase:", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_WHATSTHIS
        definitionLed->setWhatsThis(QApplication::translate("PhraseBookBox", "This is a definition for the source phrase.", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_WHATSTHIS
#ifndef QT_NO_WHATSTHIS
        sourceLed->setWhatsThis(QApplication::translate("PhraseBookBox", "This is the phrase in the source language.", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_WHATSTHIS
        definition->setText(QApplication::translate("PhraseBookBox", "&Definition:", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_WHATSTHIS
        newBut->setWhatsThis(QApplication::translate("PhraseBookBox", "Click here to add the phrase to the phrase book.", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_WHATSTHIS
        newBut->setText(QApplication::translate("PhraseBookBox", "&New Phrase", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_WHATSTHIS
        removeBut->setWhatsThis(QApplication::translate("PhraseBookBox", "Click here to remove the phrase from the phrase book.", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_WHATSTHIS
        removeBut->setText(QApplication::translate("PhraseBookBox", "&Remove Phrase", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_WHATSTHIS
        saveBut->setWhatsThis(QApplication::translate("PhraseBookBox", "Click here to save the changes made.", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_WHATSTHIS
        saveBut->setText(QApplication::translate("PhraseBookBox", "&Save", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_WHATSTHIS
        closeBut->setWhatsThis(QApplication::translate("PhraseBookBox", "Click here to close this window.", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_WHATSTHIS
        closeBut->setText(QApplication::translate("PhraseBookBox", "Close", 0, QApplication::UnicodeUTF8));
    } // retranslateUi

};

namespace Ui {
    class PhraseBookBox: public Ui_PhraseBookBox {};
} // namespace Ui

QT_END_NAMESPACE

#endif // PHRASEBOOKBOX_H

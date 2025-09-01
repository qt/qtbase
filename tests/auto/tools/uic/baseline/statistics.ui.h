/*

* Copyright (C) 2016 The Qt Company Ltd.
* SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

*/

/********************************************************************************
** Form generated from reading UI file 'statistics.ui'
**
** Created by: Qt User Interface Compiler version 6.0.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef STATISTICS_H
#define STATISTICS_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QDialog>
#include <QtWidgets/QFrame>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpacerItem>

QT_BEGIN_NAMESPACE

class Ui_linguist_stats
{
public:
    QGridLayout *unnamed;
    QHBoxLayout *unnamed1;
    QSpacerItem *spacer4_2;
    QPushButton *closeBtn;
    QSpacerItem *spacer4;
    QFrame *frame4;
    QGridLayout *unnamed2;
    QLabel *textLabel4;
    QLabel *textLabel5;
    QLabel *untrWords;
    QLabel *trWords;
    QLabel *textLabel1;
    QLabel *trChars;
    QLabel *untrChars;
    QLabel *textLabel3;
    QLabel *textLabel6;
    QLabel *trCharsSpc;
    QLabel *untrCharsSpc;

    void setupUi(QDialog *linguist_stats)
    {
        if (linguist_stats->objectName().isEmpty())
            linguist_stats->setObjectName("linguist_stats");
        linguist_stats->resize(373, 182);
        unnamed = new QGridLayout(linguist_stats);
        unnamed->setSpacing(6);
        unnamed->setContentsMargins(11, 11, 11, 11);
        unnamed->setObjectName("unnamed");
        unnamed1 = new QHBoxLayout();
        unnamed1->setSpacing(6);
        unnamed1->setObjectName("unnamed1");
        spacer4_2 = new QSpacerItem(40, 20, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        unnamed1->addItem(spacer4_2);

        closeBtn = new QPushButton(linguist_stats);
        closeBtn->setObjectName("closeBtn");

        unnamed1->addWidget(closeBtn);

        spacer4 = new QSpacerItem(40, 20, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        unnamed1->addItem(spacer4);


        unnamed->addLayout(unnamed1, 1, 0, 1, 1);

        frame4 = new QFrame(linguist_stats);
        frame4->setObjectName("frame4");
        frame4->setFrameShape(QFrame::Shape::StyledPanel);
        frame4->setFrameShadow(QFrame::Shadow::Raised);
        unnamed2 = new QGridLayout(frame4);
        unnamed2->setSpacing(6);
        unnamed2->setContentsMargins(11, 11, 11, 11);
        unnamed2->setObjectName("unnamed2");
        textLabel4 = new QLabel(frame4);
        textLabel4->setObjectName("textLabel4");

        unnamed2->addWidget(textLabel4, 0, 2, 1, 1);

        textLabel5 = new QLabel(frame4);
        textLabel5->setObjectName("textLabel5");

        unnamed2->addWidget(textLabel5, 0, 1, 1, 1);

        untrWords = new QLabel(frame4);
        untrWords->setObjectName("untrWords");

        unnamed2->addWidget(untrWords, 1, 1, 1, 1);

        trWords = new QLabel(frame4);
        trWords->setObjectName("trWords");

        unnamed2->addWidget(trWords, 1, 2, 1, 1);

        textLabel1 = new QLabel(frame4);
        textLabel1->setObjectName("textLabel1");

        unnamed2->addWidget(textLabel1, 1, 0, 1, 1);

        trChars = new QLabel(frame4);
        trChars->setObjectName("trChars");

        unnamed2->addWidget(trChars, 2, 2, 1, 1);

        untrChars = new QLabel(frame4);
        untrChars->setObjectName("untrChars");

        unnamed2->addWidget(untrChars, 2, 1, 1, 1);

        textLabel3 = new QLabel(frame4);
        textLabel3->setObjectName("textLabel3");

        unnamed2->addWidget(textLabel3, 2, 0, 1, 1);

        textLabel6 = new QLabel(frame4);
        textLabel6->setObjectName("textLabel6");

        unnamed2->addWidget(textLabel6, 3, 0, 1, 1);

        trCharsSpc = new QLabel(frame4);
        trCharsSpc->setObjectName("trCharsSpc");

        unnamed2->addWidget(trCharsSpc, 3, 2, 1, 1);

        untrCharsSpc = new QLabel(frame4);
        untrCharsSpc->setObjectName("untrCharsSpc");

        unnamed2->addWidget(untrCharsSpc, 3, 1, 1, 1);


        unnamed->addWidget(frame4, 0, 0, 1, 1);


        retranslateUi(linguist_stats);

        QMetaObject::connectSlotsByName(linguist_stats);
    } // setupUi

    void retranslateUi(QDialog *linguist_stats)
    {
        linguist_stats->setWindowTitle(QCoreApplication::translate("linguist_stats", "Statistics", nullptr));
        closeBtn->setText(QCoreApplication::translate("linguist_stats", "&Close", nullptr));
        textLabel4->setText(QCoreApplication::translate("linguist_stats", "Translation", nullptr));
        textLabel5->setText(QCoreApplication::translate("linguist_stats", "Source", nullptr));
        untrWords->setText(QCoreApplication::translate("linguist_stats", "0", nullptr));
        trWords->setText(QCoreApplication::translate("linguist_stats", "0", nullptr));
        textLabel1->setText(QCoreApplication::translate("linguist_stats", "Words:", nullptr));
        trChars->setText(QCoreApplication::translate("linguist_stats", "0", nullptr));
        untrChars->setText(QCoreApplication::translate("linguist_stats", "0", nullptr));
        textLabel3->setText(QCoreApplication::translate("linguist_stats", "Characters:", nullptr));
        textLabel6->setText(QCoreApplication::translate("linguist_stats", "Characters (with spaces):", nullptr));
        trCharsSpc->setText(QCoreApplication::translate("linguist_stats", "0", nullptr));
        untrCharsSpc->setText(QCoreApplication::translate("linguist_stats", "0", nullptr));
    } // retranslateUi

};

namespace Ui {
    class linguist_stats: public Ui_linguist_stats {};
} // namespace Ui

QT_END_NAMESPACE

#endif // STATISTICS_H

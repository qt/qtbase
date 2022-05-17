/*

* Copyright (C) 2016 The Qt Company Ltd.
* SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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

class Ui_Statistics
{
public:
    QGridLayout *gridLayout;
    QHBoxLayout *hboxLayout;
    QSpacerItem *spacer4_2;
    QPushButton *closeBtn;
    QSpacerItem *spacer4;
    QFrame *frame4;
    QGridLayout *gridLayout1;
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

    void setupUi(QDialog *Statistics)
    {
        if (Statistics->objectName().isEmpty())
            Statistics->setObjectName("Statistics");
        Statistics->setObjectName(QString::fromUtf8("linguist_stats"));
        Statistics->resize(336, 164);
        gridLayout = new QGridLayout(Statistics);
        gridLayout->setSpacing(6);
        gridLayout->setContentsMargins(11, 11, 11, 11);
        gridLayout->setObjectName("gridLayout");
        gridLayout->setObjectName(QString::fromUtf8("unnamed"));
        hboxLayout = new QHBoxLayout();
        hboxLayout->setSpacing(6);
        hboxLayout->setObjectName("hboxLayout");
        hboxLayout->setObjectName(QString::fromUtf8("unnamed"));
        spacer4_2 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        hboxLayout->addItem(spacer4_2);

        closeBtn = new QPushButton(Statistics);
        closeBtn->setObjectName("closeBtn");

        hboxLayout->addWidget(closeBtn);

        spacer4 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        hboxLayout->addItem(spacer4);


        gridLayout->addLayout(hboxLayout, 1, 0, 1, 1);

        frame4 = new QFrame(Statistics);
        frame4->setObjectName("frame4");
        frame4->setFrameShape(QFrame::StyledPanel);
        frame4->setFrameShadow(QFrame::Raised);
        gridLayout1 = new QGridLayout(frame4);
        gridLayout1->setSpacing(6);
        gridLayout1->setContentsMargins(11, 11, 11, 11);
        gridLayout1->setObjectName("gridLayout1");
        gridLayout1->setObjectName(QString::fromUtf8("unnamed"));
        textLabel4 = new QLabel(frame4);
        textLabel4->setObjectName("textLabel4");

        gridLayout1->addWidget(textLabel4, 0, 2, 1, 1);

        textLabel5 = new QLabel(frame4);
        textLabel5->setObjectName("textLabel5");

        gridLayout1->addWidget(textLabel5, 0, 1, 1, 1);

        untrWords = new QLabel(frame4);
        untrWords->setObjectName("untrWords");

        gridLayout1->addWidget(untrWords, 1, 1, 1, 1);

        trWords = new QLabel(frame4);
        trWords->setObjectName("trWords");

        gridLayout1->addWidget(trWords, 1, 2, 1, 1);

        textLabel1 = new QLabel(frame4);
        textLabel1->setObjectName("textLabel1");

        gridLayout1->addWidget(textLabel1, 1, 0, 1, 1);

        trChars = new QLabel(frame4);
        trChars->setObjectName("trChars");

        gridLayout1->addWidget(trChars, 2, 2, 1, 1);

        untrChars = new QLabel(frame4);
        untrChars->setObjectName("untrChars");

        gridLayout1->addWidget(untrChars, 2, 1, 1, 1);

        textLabel3 = new QLabel(frame4);
        textLabel3->setObjectName("textLabel3");

        gridLayout1->addWidget(textLabel3, 2, 0, 1, 1);

        textLabel6 = new QLabel(frame4);
        textLabel6->setObjectName("textLabel6");

        gridLayout1->addWidget(textLabel6, 3, 0, 1, 1);

        trCharsSpc = new QLabel(frame4);
        trCharsSpc->setObjectName("trCharsSpc");

        gridLayout1->addWidget(trCharsSpc, 3, 2, 1, 1);

        untrCharsSpc = new QLabel(frame4);
        untrCharsSpc->setObjectName("untrCharsSpc");

        gridLayout1->addWidget(untrCharsSpc, 3, 1, 1, 1);


        gridLayout->addWidget(frame4, 0, 0, 1, 1);


        retranslateUi(Statistics);

        QMetaObject::connectSlotsByName(Statistics);
    } // setupUi

    void retranslateUi(QDialog *Statistics)
    {
        Statistics->setWindowTitle(QCoreApplication::translate("Statistics", "Statistics", nullptr));
        closeBtn->setText(QCoreApplication::translate("Statistics", "&Close", nullptr));
        textLabel4->setText(QCoreApplication::translate("Statistics", "Translation", nullptr));
        textLabel5->setText(QCoreApplication::translate("Statistics", "Source", nullptr));
        untrWords->setText(QCoreApplication::translate("Statistics", "0", nullptr));
        trWords->setText(QCoreApplication::translate("Statistics", "0", nullptr));
        textLabel1->setText(QCoreApplication::translate("Statistics", "Words:", nullptr));
        trChars->setText(QCoreApplication::translate("Statistics", "0", nullptr));
        untrChars->setText(QCoreApplication::translate("Statistics", "0", nullptr));
        textLabel3->setText(QCoreApplication::translate("Statistics", "Characters:", nullptr));
        textLabel6->setText(QCoreApplication::translate("Statistics", "Characters (with spaces):", nullptr));
        trCharsSpc->setText(QCoreApplication::translate("Statistics", "0", nullptr));
        untrCharsSpc->setText(QCoreApplication::translate("Statistics", "0", nullptr));
    } // retranslateUi

};

namespace Ui {
    class Statistics: public Ui_Statistics {};
} // namespace Ui

QT_END_NAMESPACE

#endif // STATISTICS_H

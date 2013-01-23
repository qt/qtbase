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
** Form generated from reading UI file 'statistics.ui'
**
** Created by: Qt User Interface Compiler version 5.0.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef STATISTICS_H
#define STATISTICS_H

#include <QtCore/QVariant>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QDialog>
#include <QtWidgets/QFrame>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QHeaderView>
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
            Statistics->setObjectName(QStringLiteral("Statistics"));
        Statistics->setObjectName(QStringLiteral("linguist_stats"));
        Statistics->resize(336, 164);
        gridLayout = new QGridLayout(Statistics);
        gridLayout->setSpacing(6);
        gridLayout->setContentsMargins(11, 11, 11, 11);
        gridLayout->setObjectName(QStringLiteral("gridLayout"));
        gridLayout->setObjectName(QStringLiteral("unnamed"));
        hboxLayout = new QHBoxLayout();
        hboxLayout->setSpacing(6);
        hboxLayout->setObjectName(QStringLiteral("hboxLayout"));
        hboxLayout->setObjectName(QStringLiteral("unnamed"));
        spacer4_2 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        hboxLayout->addItem(spacer4_2);

        closeBtn = new QPushButton(Statistics);
        closeBtn->setObjectName(QStringLiteral("closeBtn"));

        hboxLayout->addWidget(closeBtn);

        spacer4 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        hboxLayout->addItem(spacer4);


        gridLayout->addLayout(hboxLayout, 1, 0, 1, 1);

        frame4 = new QFrame(Statistics);
        frame4->setObjectName(QStringLiteral("frame4"));
        frame4->setFrameShape(QFrame::StyledPanel);
        frame4->setFrameShadow(QFrame::Raised);
        gridLayout1 = new QGridLayout(frame4);
        gridLayout1->setSpacing(6);
        gridLayout1->setContentsMargins(11, 11, 11, 11);
        gridLayout1->setObjectName(QStringLiteral("gridLayout1"));
        gridLayout1->setObjectName(QStringLiteral("unnamed"));
        textLabel4 = new QLabel(frame4);
        textLabel4->setObjectName(QStringLiteral("textLabel4"));

        gridLayout1->addWidget(textLabel4, 0, 2, 1, 1);

        textLabel5 = new QLabel(frame4);
        textLabel5->setObjectName(QStringLiteral("textLabel5"));

        gridLayout1->addWidget(textLabel5, 0, 1, 1, 1);

        untrWords = new QLabel(frame4);
        untrWords->setObjectName(QStringLiteral("untrWords"));

        gridLayout1->addWidget(untrWords, 1, 1, 1, 1);

        trWords = new QLabel(frame4);
        trWords->setObjectName(QStringLiteral("trWords"));

        gridLayout1->addWidget(trWords, 1, 2, 1, 1);

        textLabel1 = new QLabel(frame4);
        textLabel1->setObjectName(QStringLiteral("textLabel1"));

        gridLayout1->addWidget(textLabel1, 1, 0, 1, 1);

        trChars = new QLabel(frame4);
        trChars->setObjectName(QStringLiteral("trChars"));

        gridLayout1->addWidget(trChars, 2, 2, 1, 1);

        untrChars = new QLabel(frame4);
        untrChars->setObjectName(QStringLiteral("untrChars"));

        gridLayout1->addWidget(untrChars, 2, 1, 1, 1);

        textLabel3 = new QLabel(frame4);
        textLabel3->setObjectName(QStringLiteral("textLabel3"));

        gridLayout1->addWidget(textLabel3, 2, 0, 1, 1);

        textLabel6 = new QLabel(frame4);
        textLabel6->setObjectName(QStringLiteral("textLabel6"));

        gridLayout1->addWidget(textLabel6, 3, 0, 1, 1);

        trCharsSpc = new QLabel(frame4);
        trCharsSpc->setObjectName(QStringLiteral("trCharsSpc"));

        gridLayout1->addWidget(trCharsSpc, 3, 2, 1, 1);

        untrCharsSpc = new QLabel(frame4);
        untrCharsSpc->setObjectName(QStringLiteral("untrCharsSpc"));

        gridLayout1->addWidget(untrCharsSpc, 3, 1, 1, 1);


        gridLayout->addWidget(frame4, 0, 0, 1, 1);


        retranslateUi(Statistics);

        QMetaObject::connectSlotsByName(Statistics);
    } // setupUi

    void retranslateUi(QDialog *Statistics)
    {
        Statistics->setWindowTitle(QApplication::translate("Statistics", "Statistics", 0));
        closeBtn->setText(QApplication::translate("Statistics", "&Close", 0));
        textLabel4->setText(QApplication::translate("Statistics", "Translation", 0));
        textLabel5->setText(QApplication::translate("Statistics", "Source", 0));
        untrWords->setText(QApplication::translate("Statistics", "0", 0));
        trWords->setText(QApplication::translate("Statistics", "0", 0));
        textLabel1->setText(QApplication::translate("Statistics", "Words:", 0));
        trChars->setText(QApplication::translate("Statistics", "0", 0));
        untrChars->setText(QApplication::translate("Statistics", "0", 0));
        textLabel3->setText(QApplication::translate("Statistics", "Characters:", 0));
        textLabel6->setText(QApplication::translate("Statistics", "Characters (with spaces):", 0));
        trCharsSpc->setText(QApplication::translate("Statistics", "0", 0));
        untrCharsSpc->setText(QApplication::translate("Statistics", "0", 0));
    } // retranslateUi

};

namespace Ui {
    class Statistics: public Ui_Statistics {};
} // namespace Ui

QT_END_NAMESPACE

#endif // STATISTICS_H

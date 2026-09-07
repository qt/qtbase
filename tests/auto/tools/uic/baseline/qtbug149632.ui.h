/*

* Copyright (C) 2026 The Qt Company Ltd.
* SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

*/

/********************************************************************************
** Form generated from reading UI file 'qtbug149632.ui'
**
** Created by: Qt User Interface Compiler version 6.13.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef QTBUG149632_H
#define QTBUG149632_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QLabel>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_QtBug149632
{
public:
    QVBoxLayout *verticalLayout;
    QLabel *wrappedSet;
    QLabel *wrappedEnum;
    QLabel *paddedSet;

    void setupUi(QWidget *QtBug149632)
    {
        if (QtBug149632->objectName().isEmpty())
            QtBug149632->setObjectName("QtBug149632");
        QtBug149632->resize(200, 120);
        verticalLayout = new QVBoxLayout(QtBug149632);
        verticalLayout->setObjectName("verticalLayout");
        wrappedSet = new QLabel(QtBug149632);
        wrappedSet->setObjectName("wrappedSet");
        wrappedSet->setAlignment(Qt::AlignmentFlag::AlignRight|Qt::AlignmentFlag::AlignTrailing|Qt::AlignmentFlag::AlignVCenter);

        verticalLayout->addWidget(wrappedSet);

        wrappedEnum = new QLabel(QtBug149632);
        wrappedEnum->setObjectName("wrappedEnum");
        wrappedEnum->setFrameShape(QFrame::Shape::Box);

        verticalLayout->addWidget(wrappedEnum);

        paddedSet = new QLabel(QtBug149632);
        paddedSet->setObjectName("paddedSet");
        paddedSet->setAlignment(Qt::AlignmentFlag::AlignLeft|Qt::AlignmentFlag::AlignTop);

        verticalLayout->addWidget(paddedSet);


        retranslateUi(QtBug149632);

        QMetaObject::connectSlotsByName(QtBug149632);
    } // setupUi

    void retranslateUi(QWidget *QtBug149632)
    {
        (void)QtBug149632;
    } // retranslateUi

};

namespace Ui {
    class QtBug149632: public Ui_QtBug149632 {};
} // namespace Ui

QT_END_NAMESPACE

#endif // QTBUG149632_H

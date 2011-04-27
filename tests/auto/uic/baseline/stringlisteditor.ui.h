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
** Form generated from reading UI file 'stringlisteditor.ui'
**
** Created: Fri Sep 4 10:17:14 2009
**      by: Qt User Interface Compiler version 4.6.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef STRINGLISTEDITOR_H
#define STRINGLISTEDITOR_H

#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QDialog>
#include <QtGui/QDialogButtonBox>
#include <QtGui/QGridLayout>
#include <QtGui/QGroupBox>
#include <QtGui/QHBoxLayout>
#include <QtGui/QHeaderView>
#include <QtGui/QLabel>
#include <QtGui/QLineEdit>
#include <QtGui/QListView>
#include <QtGui/QSpacerItem>
#include <QtGui/QToolButton>
#include <QtGui/QVBoxLayout>

QT_BEGIN_NAMESPACE

namespace qdesigner_internal {

class Ui_Dialog
{
public:
    QVBoxLayout *vboxLayout;
    QGroupBox *groupBox;
    QGridLayout *gridLayout;
    QVBoxLayout *vboxLayout1;
    QHBoxLayout *hboxLayout;
    QToolButton *newButton;
    QToolButton *deleteButton;
    QSpacerItem *spacerItem;
    QHBoxLayout *hboxLayout1;
    QLabel *label;
    QLineEdit *valueEdit;
    QVBoxLayout *vboxLayout2;
    QSpacerItem *spacerItem1;
    QToolButton *upButton;
    QToolButton *downButton;
    QSpacerItem *spacerItem2;
    QListView *listView;
    QDialogButtonBox *buttonBox;

    void setupUi(QDialog *qdesigner_internal__Dialog)
    {
        if (qdesigner_internal__Dialog->objectName().isEmpty())
            qdesigner_internal__Dialog->setObjectName(QString::fromUtf8("qdesigner_internal__Dialog"));
        qdesigner_internal__Dialog->resize(400, 300);
        vboxLayout = new QVBoxLayout(qdesigner_internal__Dialog);
#ifndef Q_OS_MAC
        vboxLayout->setSpacing(6);
#endif
#ifndef Q_OS_MAC
        vboxLayout->setContentsMargins(9, 9, 9, 9);
#endif
        vboxLayout->setObjectName(QString::fromUtf8("vboxLayout"));
        groupBox = new QGroupBox(qdesigner_internal__Dialog);
        groupBox->setObjectName(QString::fromUtf8("groupBox"));
        gridLayout = new QGridLayout(groupBox);
#ifndef Q_OS_MAC
        gridLayout->setSpacing(6);
#endif
#ifndef Q_OS_MAC
        gridLayout->setContentsMargins(9, 9, 9, 9);
#endif
        gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
        vboxLayout1 = new QVBoxLayout();
#ifndef Q_OS_MAC
        vboxLayout1->setSpacing(6);
#endif
#ifndef Q_OS_MAC
        vboxLayout1->setContentsMargins(0, 0, 0, 0);
#endif
        vboxLayout1->setObjectName(QString::fromUtf8("vboxLayout1"));
        hboxLayout = new QHBoxLayout();
#ifndef Q_OS_MAC
        hboxLayout->setSpacing(6);
#endif
#ifndef Q_OS_MAC
        hboxLayout->setContentsMargins(0, 0, 0, 0);
#endif
        hboxLayout->setObjectName(QString::fromUtf8("hboxLayout"));
        newButton = new QToolButton(groupBox);
        newButton->setObjectName(QString::fromUtf8("newButton"));
        newButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);

        hboxLayout->addWidget(newButton);

        deleteButton = new QToolButton(groupBox);
        deleteButton->setObjectName(QString::fromUtf8("deleteButton"));
        deleteButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);

        hboxLayout->addWidget(deleteButton);

        spacerItem = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        hboxLayout->addItem(spacerItem);


        vboxLayout1->addLayout(hboxLayout);

        hboxLayout1 = new QHBoxLayout();
#ifndef Q_OS_MAC
        hboxLayout1->setSpacing(6);
#endif
        hboxLayout1->setContentsMargins(0, 0, 0, 0);
        hboxLayout1->setObjectName(QString::fromUtf8("hboxLayout1"));
        label = new QLabel(groupBox);
        label->setObjectName(QString::fromUtf8("label"));

        hboxLayout1->addWidget(label);

        valueEdit = new QLineEdit(groupBox);
        valueEdit->setObjectName(QString::fromUtf8("valueEdit"));

        hboxLayout1->addWidget(valueEdit);


        vboxLayout1->addLayout(hboxLayout1);


        gridLayout->addLayout(vboxLayout1, 1, 0, 1, 2);

        vboxLayout2 = new QVBoxLayout();
#ifndef Q_OS_MAC
        vboxLayout2->setSpacing(6);
#endif
        vboxLayout2->setContentsMargins(0, 0, 0, 0);
        vboxLayout2->setObjectName(QString::fromUtf8("vboxLayout2"));
        spacerItem1 = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

        vboxLayout2->addItem(spacerItem1);

        upButton = new QToolButton(groupBox);
        upButton->setObjectName(QString::fromUtf8("upButton"));

        vboxLayout2->addWidget(upButton);

        downButton = new QToolButton(groupBox);
        downButton->setObjectName(QString::fromUtf8("downButton"));

        vboxLayout2->addWidget(downButton);

        spacerItem2 = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

        vboxLayout2->addItem(spacerItem2);


        gridLayout->addLayout(vboxLayout2, 0, 1, 1, 1);

        listView = new QListView(groupBox);
        listView->setObjectName(QString::fromUtf8("listView"));

        gridLayout->addWidget(listView, 0, 0, 1, 1);


        vboxLayout->addWidget(groupBox);

        buttonBox = new QDialogButtonBox(qdesigner_internal__Dialog);
        buttonBox->setObjectName(QString::fromUtf8("buttonBox"));
        buttonBox->setOrientation(Qt::Horizontal);
        buttonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::NoButton|QDialogButtonBox::Ok);

        vboxLayout->addWidget(buttonBox);

#ifndef QT_NO_SHORTCUT
        label->setBuddy(valueEdit);
#endif // QT_NO_SHORTCUT

        retranslateUi(qdesigner_internal__Dialog);
        QObject::connect(buttonBox, SIGNAL(accepted()), qdesigner_internal__Dialog, SLOT(accept()));
        QObject::connect(buttonBox, SIGNAL(rejected()), qdesigner_internal__Dialog, SLOT(reject()));

        QMetaObject::connectSlotsByName(qdesigner_internal__Dialog);
    } // setupUi

    void retranslateUi(QDialog *qdesigner_internal__Dialog)
    {
        qdesigner_internal__Dialog->setWindowTitle(QApplication::translate("qdesigner_internal::Dialog", "Dialog", 0, QApplication::UnicodeUTF8));
        groupBox->setTitle(QApplication::translate("qdesigner_internal::Dialog", "StringList", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        newButton->setToolTip(QApplication::translate("qdesigner_internal::Dialog", "New String", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        newButton->setText(QApplication::translate("qdesigner_internal::Dialog", "&New", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        deleteButton->setToolTip(QApplication::translate("qdesigner_internal::Dialog", "Delete String", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        deleteButton->setText(QApplication::translate("qdesigner_internal::Dialog", "&Delete", 0, QApplication::UnicodeUTF8));
        label->setText(QApplication::translate("qdesigner_internal::Dialog", "&Value:", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        upButton->setToolTip(QApplication::translate("qdesigner_internal::Dialog", "Move String Up", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        upButton->setText(QApplication::translate("qdesigner_internal::Dialog", "Up", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        downButton->setToolTip(QApplication::translate("qdesigner_internal::Dialog", "Move String Down", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        downButton->setText(QApplication::translate("qdesigner_internal::Dialog", "Down", 0, QApplication::UnicodeUTF8));
    } // retranslateUi

};

} // namespace qdesigner_internal

namespace qdesigner_internal {
namespace Ui {
    class Dialog: public Ui_Dialog {};
} // namespace Ui
} // namespace qdesigner_internal

QT_END_NAMESPACE

#endif // STRINGLISTEDITOR_H

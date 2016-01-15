/*
*********************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the autotests of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
*********************************************************************
*/

/********************************************************************************
** Form generated from reading UI file 'stringlisteditor.ui'
**
** Created by: Qt User Interface Compiler version 5.0.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef STRINGLISTEDITOR_H
#define STRINGLISTEDITOR_H

#include <QtCore/QVariant>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QDialog>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QListView>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QToolButton>
#include <QtWidgets/QVBoxLayout>

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
            qdesigner_internal__Dialog->setObjectName(QStringLiteral("qdesigner_internal__Dialog"));
        qdesigner_internal__Dialog->resize(400, 300);
        vboxLayout = new QVBoxLayout(qdesigner_internal__Dialog);
#ifndef Q_OS_MAC
        vboxLayout->setSpacing(6);
#endif
#ifndef Q_OS_MAC
        vboxLayout->setContentsMargins(9, 9, 9, 9);
#endif
        vboxLayout->setObjectName(QStringLiteral("vboxLayout"));
        groupBox = new QGroupBox(qdesigner_internal__Dialog);
        groupBox->setObjectName(QStringLiteral("groupBox"));
        gridLayout = new QGridLayout(groupBox);
#ifndef Q_OS_MAC
        gridLayout->setSpacing(6);
#endif
#ifndef Q_OS_MAC
        gridLayout->setContentsMargins(9, 9, 9, 9);
#endif
        gridLayout->setObjectName(QStringLiteral("gridLayout"));
        vboxLayout1 = new QVBoxLayout();
#ifndef Q_OS_MAC
        vboxLayout1->setSpacing(6);
#endif
#ifndef Q_OS_MAC
        vboxLayout1->setContentsMargins(0, 0, 0, 0);
#endif
        vboxLayout1->setObjectName(QStringLiteral("vboxLayout1"));
        hboxLayout = new QHBoxLayout();
#ifndef Q_OS_MAC
        hboxLayout->setSpacing(6);
#endif
#ifndef Q_OS_MAC
        hboxLayout->setContentsMargins(0, 0, 0, 0);
#endif
        hboxLayout->setObjectName(QStringLiteral("hboxLayout"));
        newButton = new QToolButton(groupBox);
        newButton->setObjectName(QStringLiteral("newButton"));
        newButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);

        hboxLayout->addWidget(newButton);

        deleteButton = new QToolButton(groupBox);
        deleteButton->setObjectName(QStringLiteral("deleteButton"));
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
        hboxLayout1->setObjectName(QStringLiteral("hboxLayout1"));
        label = new QLabel(groupBox);
        label->setObjectName(QStringLiteral("label"));

        hboxLayout1->addWidget(label);

        valueEdit = new QLineEdit(groupBox);
        valueEdit->setObjectName(QStringLiteral("valueEdit"));

        hboxLayout1->addWidget(valueEdit);


        vboxLayout1->addLayout(hboxLayout1);


        gridLayout->addLayout(vboxLayout1, 1, 0, 1, 2);

        vboxLayout2 = new QVBoxLayout();
#ifndef Q_OS_MAC
        vboxLayout2->setSpacing(6);
#endif
        vboxLayout2->setContentsMargins(0, 0, 0, 0);
        vboxLayout2->setObjectName(QStringLiteral("vboxLayout2"));
        spacerItem1 = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

        vboxLayout2->addItem(spacerItem1);

        upButton = new QToolButton(groupBox);
        upButton->setObjectName(QStringLiteral("upButton"));

        vboxLayout2->addWidget(upButton);

        downButton = new QToolButton(groupBox);
        downButton->setObjectName(QStringLiteral("downButton"));

        vboxLayout2->addWidget(downButton);

        spacerItem2 = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

        vboxLayout2->addItem(spacerItem2);


        gridLayout->addLayout(vboxLayout2, 0, 1, 1, 1);

        listView = new QListView(groupBox);
        listView->setObjectName(QStringLiteral("listView"));

        gridLayout->addWidget(listView, 0, 0, 1, 1);


        vboxLayout->addWidget(groupBox);

        buttonBox = new QDialogButtonBox(qdesigner_internal__Dialog);
        buttonBox->setObjectName(QStringLiteral("buttonBox"));
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
        qdesigner_internal__Dialog->setWindowTitle(QApplication::translate("qdesigner_internal::Dialog", "Dialog", 0));
        groupBox->setTitle(QApplication::translate("qdesigner_internal::Dialog", "StringList", 0));
#ifndef QT_NO_TOOLTIP
        newButton->setToolTip(QApplication::translate("qdesigner_internal::Dialog", "New String", 0));
#endif // QT_NO_TOOLTIP
        newButton->setText(QApplication::translate("qdesigner_internal::Dialog", "&New", 0));
#ifndef QT_NO_TOOLTIP
        deleteButton->setToolTip(QApplication::translate("qdesigner_internal::Dialog", "Delete String", 0));
#endif // QT_NO_TOOLTIP
        deleteButton->setText(QApplication::translate("qdesigner_internal::Dialog", "&Delete", 0));
        label->setText(QApplication::translate("qdesigner_internal::Dialog", "&Value:", 0));
#ifndef QT_NO_TOOLTIP
        upButton->setToolTip(QApplication::translate("qdesigner_internal::Dialog", "Move String Up", 0));
#endif // QT_NO_TOOLTIP
        upButton->setText(QApplication::translate("qdesigner_internal::Dialog", "Up", 0));
#ifndef QT_NO_TOOLTIP
        downButton->setToolTip(QApplication::translate("qdesigner_internal::Dialog", "Move String Down", 0));
#endif // QT_NO_TOOLTIP
        downButton->setText(QApplication::translate("qdesigner_internal::Dialog", "Down", 0));
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

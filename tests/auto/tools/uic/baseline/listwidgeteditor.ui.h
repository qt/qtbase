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
** Form generated from reading UI file 'listwidgeteditor.ui'
**
** Created by: Qt User Interface Compiler version 5.12.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef LISTWIDGETEDITOR_H
#define LISTWIDGETEDITOR_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QDialog>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QToolButton>
#include <QtWidgets/QVBoxLayout>
#include "iconselector_p.h"

QT_BEGIN_NAMESPACE

namespace qdesigner_internal {

class Ui_ListWidgetEditor
{
public:
    QVBoxLayout *vboxLayout;
    QGroupBox *groupBox;
    QGridLayout *gridLayout;
    QListWidget *listWidget;
    QHBoxLayout *horizontalLayout_2;
    QToolButton *newItemButton;
    QToolButton *deleteItemButton;
    QSpacerItem *spacerItem;
    QToolButton *moveItemUpButton;
    QToolButton *moveItemDownButton;
    QHBoxLayout *horizontalLayout;
    QLabel *label;
    qdesigner_internal::IconSelector *itemIconSelector;
    QSpacerItem *horizontalSpacer;
    QDialogButtonBox *buttonBox;

    void setupUi(QDialog *qdesigner_internal__ListWidgetEditor)
    {
        if (qdesigner_internal__ListWidgetEditor->objectName().isEmpty())
            qdesigner_internal__ListWidgetEditor->setObjectName(QString::fromUtf8("qdesigner_internal__ListWidgetEditor"));
        qdesigner_internal__ListWidgetEditor->resize(223, 245);
        vboxLayout = new QVBoxLayout(qdesigner_internal__ListWidgetEditor);
#ifndef Q_OS_MAC
        vboxLayout->setSpacing(6);
#endif
#ifndef Q_OS_MAC
        vboxLayout->setContentsMargins(9, 9, 9, 9);
#endif
        vboxLayout->setObjectName(QString::fromUtf8("vboxLayout"));
        groupBox = new QGroupBox(qdesigner_internal__ListWidgetEditor);
        groupBox->setObjectName(QString::fromUtf8("groupBox"));
        gridLayout = new QGridLayout(groupBox);
        gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
        listWidget = new QListWidget(groupBox);
        listWidget->setObjectName(QString::fromUtf8("listWidget"));

        gridLayout->addWidget(listWidget, 0, 0, 1, 1);

        horizontalLayout_2 = new QHBoxLayout();
        horizontalLayout_2->setObjectName(QString::fromUtf8("horizontalLayout_2"));
        newItemButton = new QToolButton(groupBox);
        newItemButton->setObjectName(QString::fromUtf8("newItemButton"));

        horizontalLayout_2->addWidget(newItemButton);

        deleteItemButton = new QToolButton(groupBox);
        deleteItemButton->setObjectName(QString::fromUtf8("deleteItemButton"));

        horizontalLayout_2->addWidget(deleteItemButton);

        spacerItem = new QSpacerItem(16, 10, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_2->addItem(spacerItem);

        moveItemUpButton = new QToolButton(groupBox);
        moveItemUpButton->setObjectName(QString::fromUtf8("moveItemUpButton"));

        horizontalLayout_2->addWidget(moveItemUpButton);

        moveItemDownButton = new QToolButton(groupBox);
        moveItemDownButton->setObjectName(QString::fromUtf8("moveItemDownButton"));

        horizontalLayout_2->addWidget(moveItemDownButton);


        gridLayout->addLayout(horizontalLayout_2, 1, 0, 1, 1);

        horizontalLayout = new QHBoxLayout();
        horizontalLayout->setObjectName(QString::fromUtf8("horizontalLayout"));
        label = new QLabel(groupBox);
        label->setObjectName(QString::fromUtf8("label"));

        horizontalLayout->addWidget(label);

        itemIconSelector = new qdesigner_internal::IconSelector(groupBox);
        itemIconSelector->setObjectName(QString::fromUtf8("itemIconSelector"));

        horizontalLayout->addWidget(itemIconSelector);

        horizontalSpacer = new QSpacerItem(108, 21, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout->addItem(horizontalSpacer);


        gridLayout->addLayout(horizontalLayout, 2, 0, 1, 1);


        vboxLayout->addWidget(groupBox);

        buttonBox = new QDialogButtonBox(qdesigner_internal__ListWidgetEditor);
        buttonBox->setObjectName(QString::fromUtf8("buttonBox"));
        buttonBox->setOrientation(Qt::Horizontal);
        buttonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);

        vboxLayout->addWidget(buttonBox);

        buttonBox->raise();
        groupBox->raise();
        QWidget::setTabOrder(listWidget, newItemButton);
        QWidget::setTabOrder(newItemButton, deleteItemButton);
        QWidget::setTabOrder(deleteItemButton, moveItemUpButton);
        QWidget::setTabOrder(moveItemUpButton, moveItemDownButton);

        retranslateUi(qdesigner_internal__ListWidgetEditor);
        QObject::connect(buttonBox, SIGNAL(accepted()), qdesigner_internal__ListWidgetEditor, SLOT(accept()));
        QObject::connect(buttonBox, SIGNAL(rejected()), qdesigner_internal__ListWidgetEditor, SLOT(reject()));

        QMetaObject::connectSlotsByName(qdesigner_internal__ListWidgetEditor);
    } // setupUi

    void retranslateUi(QDialog *qdesigner_internal__ListWidgetEditor)
    {
        qdesigner_internal__ListWidgetEditor->setWindowTitle(QCoreApplication::translate("qdesigner_internal::ListWidgetEditor", "Dialog", nullptr));
        groupBox->setTitle(QCoreApplication::translate("qdesigner_internal::ListWidgetEditor", "Items List", nullptr));
#if QT_CONFIG(tooltip)
        listWidget->setToolTip(QCoreApplication::translate("qdesigner_internal::ListWidgetEditor", "Items List", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(tooltip)
        newItemButton->setToolTip(QCoreApplication::translate("qdesigner_internal::ListWidgetEditor", "New Item", nullptr));
#endif // QT_CONFIG(tooltip)
        newItemButton->setText(QCoreApplication::translate("qdesigner_internal::ListWidgetEditor", "&New", nullptr));
#if QT_CONFIG(tooltip)
        deleteItemButton->setToolTip(QCoreApplication::translate("qdesigner_internal::ListWidgetEditor", "Delete Item", nullptr));
#endif // QT_CONFIG(tooltip)
        deleteItemButton->setText(QCoreApplication::translate("qdesigner_internal::ListWidgetEditor", "&Delete", nullptr));
#if QT_CONFIG(tooltip)
        moveItemUpButton->setToolTip(QCoreApplication::translate("qdesigner_internal::ListWidgetEditor", "Move Item Up", nullptr));
#endif // QT_CONFIG(tooltip)
        moveItemUpButton->setText(QCoreApplication::translate("qdesigner_internal::ListWidgetEditor", "U", nullptr));
#if QT_CONFIG(tooltip)
        moveItemDownButton->setToolTip(QCoreApplication::translate("qdesigner_internal::ListWidgetEditor", "Move Item Down", nullptr));
#endif // QT_CONFIG(tooltip)
        moveItemDownButton->setText(QCoreApplication::translate("qdesigner_internal::ListWidgetEditor", "D", nullptr));
        label->setText(QCoreApplication::translate("qdesigner_internal::ListWidgetEditor", "Icon", nullptr));
    } // retranslateUi

};

} // namespace qdesigner_internal

namespace qdesigner_internal {
namespace Ui {
    class ListWidgetEditor: public Ui_ListWidgetEditor {};
} // namespace Ui
} // namespace qdesigner_internal

QT_END_NAMESPACE

#endif // LISTWIDGETEDITOR_H

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
** Form generated from reading UI file 'listwidgeteditor.ui'
**
** Created by: Qt User Interface Compiler version 5.0.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef LISTWIDGETEDITOR_H
#define LISTWIDGETEDITOR_H

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
            qdesigner_internal__ListWidgetEditor->setObjectName(QStringLiteral("qdesigner_internal__ListWidgetEditor"));
        qdesigner_internal__ListWidgetEditor->resize(223, 245);
        vboxLayout = new QVBoxLayout(qdesigner_internal__ListWidgetEditor);
#ifndef Q_OS_MAC
        vboxLayout->setSpacing(6);
#endif
#ifndef Q_OS_MAC
        vboxLayout->setContentsMargins(9, 9, 9, 9);
#endif
        vboxLayout->setObjectName(QStringLiteral("vboxLayout"));
        groupBox = new QGroupBox(qdesigner_internal__ListWidgetEditor);
        groupBox->setObjectName(QStringLiteral("groupBox"));
        gridLayout = new QGridLayout(groupBox);
        gridLayout->setObjectName(QStringLiteral("gridLayout"));
        listWidget = new QListWidget(groupBox);
        listWidget->setObjectName(QStringLiteral("listWidget"));

        gridLayout->addWidget(listWidget, 0, 0, 1, 1);

        horizontalLayout_2 = new QHBoxLayout();
        horizontalLayout_2->setObjectName(QStringLiteral("horizontalLayout_2"));
        newItemButton = new QToolButton(groupBox);
        newItemButton->setObjectName(QStringLiteral("newItemButton"));

        horizontalLayout_2->addWidget(newItemButton);

        deleteItemButton = new QToolButton(groupBox);
        deleteItemButton->setObjectName(QStringLiteral("deleteItemButton"));

        horizontalLayout_2->addWidget(deleteItemButton);

        spacerItem = new QSpacerItem(16, 10, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_2->addItem(spacerItem);

        moveItemUpButton = new QToolButton(groupBox);
        moveItemUpButton->setObjectName(QStringLiteral("moveItemUpButton"));

        horizontalLayout_2->addWidget(moveItemUpButton);

        moveItemDownButton = new QToolButton(groupBox);
        moveItemDownButton->setObjectName(QStringLiteral("moveItemDownButton"));

        horizontalLayout_2->addWidget(moveItemDownButton);


        gridLayout->addLayout(horizontalLayout_2, 1, 0, 1, 1);

        horizontalLayout = new QHBoxLayout();
        horizontalLayout->setObjectName(QStringLiteral("horizontalLayout"));
        label = new QLabel(groupBox);
        label->setObjectName(QStringLiteral("label"));

        horizontalLayout->addWidget(label);

        itemIconSelector = new qdesigner_internal::IconSelector(groupBox);
        itemIconSelector->setObjectName(QStringLiteral("itemIconSelector"));

        horizontalLayout->addWidget(itemIconSelector);

        horizontalSpacer = new QSpacerItem(108, 21, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout->addItem(horizontalSpacer);


        gridLayout->addLayout(horizontalLayout, 2, 0, 1, 1);


        vboxLayout->addWidget(groupBox);

        buttonBox = new QDialogButtonBox(qdesigner_internal__ListWidgetEditor);
        buttonBox->setObjectName(QStringLiteral("buttonBox"));
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
        qdesigner_internal__ListWidgetEditor->setWindowTitle(QApplication::translate("qdesigner_internal::ListWidgetEditor", "Dialog", 0));
        groupBox->setTitle(QApplication::translate("qdesigner_internal::ListWidgetEditor", "Items List", 0));
#ifndef QT_NO_TOOLTIP
        listWidget->setToolTip(QApplication::translate("qdesigner_internal::ListWidgetEditor", "Items List", 0));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_TOOLTIP
        newItemButton->setToolTip(QApplication::translate("qdesigner_internal::ListWidgetEditor", "New Item", 0));
#endif // QT_NO_TOOLTIP
        newItemButton->setText(QApplication::translate("qdesigner_internal::ListWidgetEditor", "&New", 0));
#ifndef QT_NO_TOOLTIP
        deleteItemButton->setToolTip(QApplication::translate("qdesigner_internal::ListWidgetEditor", "Delete Item", 0));
#endif // QT_NO_TOOLTIP
        deleteItemButton->setText(QApplication::translate("qdesigner_internal::ListWidgetEditor", "&Delete", 0));
#ifndef QT_NO_TOOLTIP
        moveItemUpButton->setToolTip(QApplication::translate("qdesigner_internal::ListWidgetEditor", "Move Item Up", 0));
#endif // QT_NO_TOOLTIP
        moveItemUpButton->setText(QApplication::translate("qdesigner_internal::ListWidgetEditor", "U", 0));
#ifndef QT_NO_TOOLTIP
        moveItemDownButton->setToolTip(QApplication::translate("qdesigner_internal::ListWidgetEditor", "Move Item Down", 0));
#endif // QT_NO_TOOLTIP
        moveItemDownButton->setText(QApplication::translate("qdesigner_internal::ListWidgetEditor", "D", 0));
        label->setText(QApplication::translate("qdesigner_internal::ListWidgetEditor", "Icon", 0));
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

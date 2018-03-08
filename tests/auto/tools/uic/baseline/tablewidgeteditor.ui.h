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
** Form generated from reading UI file 'tablewidgeteditor.ui'
**
** Created by: Qt User Interface Compiler version 5.0.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef TABLEWIDGETEDITOR_H
#define TABLEWIDGETEDITOR_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QDialog>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QTableWidget>
#include <QtWidgets/QToolButton>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>
#include "iconselector_p.h"

QT_BEGIN_NAMESPACE

namespace qdesigner_internal {

class Ui_TableWidgetEditor
{
public:
    QGridLayout *gridLayout_4;
    QGroupBox *itemsBox;
    QGridLayout *gridLayout;
    QTableWidget *tableWidget;
    QHBoxLayout *horizontalLayout_5;
    QLabel *label_3;
    qdesigner_internal::IconSelector *itemIconSelector;
    QSpacerItem *horizontalSpacer;
    QDialogButtonBox *buttonBox;
    QWidget *widget;
    QVBoxLayout *verticalLayout;
    QGroupBox *columnsBox;
    QGridLayout *gridLayout_2;
    QListWidget *columnsListWidget;
    QHBoxLayout *horizontalLayout_3;
    QToolButton *newColumnButton;
    QToolButton *deleteColumnButton;
    QSpacerItem *spacerItem;
    QToolButton *moveColumnUpButton;
    QToolButton *moveColumnDownButton;
    QHBoxLayout *horizontalLayout_2;
    QLabel *label;
    qdesigner_internal::IconSelector *columnIconSelector;
    QSpacerItem *spacerItem1;
    QGroupBox *rowsBox;
    QGridLayout *gridLayout_3;
    QListWidget *rowsListWidget;
    QHBoxLayout *horizontalLayout_4;
    QToolButton *newRowButton;
    QToolButton *deleteRowButton;
    QSpacerItem *spacerItem2;
    QToolButton *moveRowUpButton;
    QToolButton *moveRowDownButton;
    QHBoxLayout *horizontalLayout;
    QLabel *label_2;
    qdesigner_internal::IconSelector *rowIconSelector;
    QSpacerItem *spacerItem3;

    void setupUi(QDialog *qdesigner_internal__TableWidgetEditor)
    {
        if (qdesigner_internal__TableWidgetEditor->objectName().isEmpty())
            qdesigner_internal__TableWidgetEditor->setObjectName(QStringLiteral("qdesigner_internal__TableWidgetEditor"));
        qdesigner_internal__TableWidgetEditor->resize(591, 455);
        gridLayout_4 = new QGridLayout(qdesigner_internal__TableWidgetEditor);
        gridLayout_4->setObjectName(QStringLiteral("gridLayout_4"));
        itemsBox = new QGroupBox(qdesigner_internal__TableWidgetEditor);
        itemsBox->setObjectName(QStringLiteral("itemsBox"));
        gridLayout = new QGridLayout(itemsBox);
        gridLayout->setObjectName(QStringLiteral("gridLayout"));
        tableWidget = new QTableWidget(itemsBox);
        tableWidget->setObjectName(QStringLiteral("tableWidget"));

        gridLayout->addWidget(tableWidget, 0, 0, 1, 1);

        horizontalLayout_5 = new QHBoxLayout();
        horizontalLayout_5->setObjectName(QStringLiteral("horizontalLayout_5"));
        label_3 = new QLabel(itemsBox);
        label_3->setObjectName(QStringLiteral("label_3"));

        horizontalLayout_5->addWidget(label_3);

        itemIconSelector = new qdesigner_internal::IconSelector(itemsBox);
        itemIconSelector->setObjectName(QStringLiteral("itemIconSelector"));

        horizontalLayout_5->addWidget(itemIconSelector);

        horizontalSpacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_5->addItem(horizontalSpacer);


        gridLayout->addLayout(horizontalLayout_5, 1, 0, 1, 1);


        gridLayout_4->addWidget(itemsBox, 0, 0, 1, 1);

        buttonBox = new QDialogButtonBox(qdesigner_internal__TableWidgetEditor);
        buttonBox->setObjectName(QStringLiteral("buttonBox"));
        buttonBox->setOrientation(Qt::Horizontal);
        buttonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);

        gridLayout_4->addWidget(buttonBox, 1, 0, 1, 2);

        widget = new QWidget(qdesigner_internal__TableWidgetEditor);
        widget->setObjectName(QStringLiteral("widget"));
        QSizePolicy sizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(widget->sizePolicy().hasHeightForWidth());
        widget->setSizePolicy(sizePolicy);
        verticalLayout = new QVBoxLayout(widget);
        verticalLayout->setContentsMargins(0, 0, 0, 0);
        verticalLayout->setObjectName(QStringLiteral("verticalLayout"));
        columnsBox = new QGroupBox(widget);
        columnsBox->setObjectName(QStringLiteral("columnsBox"));
        gridLayout_2 = new QGridLayout(columnsBox);
        gridLayout_2->setObjectName(QStringLiteral("gridLayout_2"));
        columnsListWidget = new QListWidget(columnsBox);
        columnsListWidget->setObjectName(QStringLiteral("columnsListWidget"));
        QSizePolicy sizePolicy1(QSizePolicy::Ignored, QSizePolicy::Expanding);
        sizePolicy1.setHorizontalStretch(0);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(columnsListWidget->sizePolicy().hasHeightForWidth());
        columnsListWidget->setSizePolicy(sizePolicy1);
        columnsListWidget->setFocusPolicy(Qt::TabFocus);

        gridLayout_2->addWidget(columnsListWidget, 0, 0, 1, 1);

        horizontalLayout_3 = new QHBoxLayout();
        horizontalLayout_3->setObjectName(QStringLiteral("horizontalLayout_3"));
        newColumnButton = new QToolButton(columnsBox);
        newColumnButton->setObjectName(QStringLiteral("newColumnButton"));

        horizontalLayout_3->addWidget(newColumnButton);

        deleteColumnButton = new QToolButton(columnsBox);
        deleteColumnButton->setObjectName(QStringLiteral("deleteColumnButton"));

        horizontalLayout_3->addWidget(deleteColumnButton);

        spacerItem = new QSpacerItem(0, 23, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_3->addItem(spacerItem);

        moveColumnUpButton = new QToolButton(columnsBox);
        moveColumnUpButton->setObjectName(QStringLiteral("moveColumnUpButton"));

        horizontalLayout_3->addWidget(moveColumnUpButton);

        moveColumnDownButton = new QToolButton(columnsBox);
        moveColumnDownButton->setObjectName(QStringLiteral("moveColumnDownButton"));

        horizontalLayout_3->addWidget(moveColumnDownButton);


        gridLayout_2->addLayout(horizontalLayout_3, 1, 0, 1, 1);

        horizontalLayout_2 = new QHBoxLayout();
        horizontalLayout_2->setObjectName(QStringLiteral("horizontalLayout_2"));
        label = new QLabel(columnsBox);
        label->setObjectName(QStringLiteral("label"));

        horizontalLayout_2->addWidget(label);

        columnIconSelector = new qdesigner_internal::IconSelector(columnsBox);
        columnIconSelector->setObjectName(QStringLiteral("columnIconSelector"));

        horizontalLayout_2->addWidget(columnIconSelector);

        spacerItem1 = new QSpacerItem(0, 21, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_2->addItem(spacerItem1);


        gridLayout_2->addLayout(horizontalLayout_2, 2, 0, 1, 1);


        verticalLayout->addWidget(columnsBox);

        rowsBox = new QGroupBox(widget);
        rowsBox->setObjectName(QStringLiteral("rowsBox"));
        gridLayout_3 = new QGridLayout(rowsBox);
        gridLayout_3->setObjectName(QStringLiteral("gridLayout_3"));
        rowsListWidget = new QListWidget(rowsBox);
        rowsListWidget->setObjectName(QStringLiteral("rowsListWidget"));
        sizePolicy1.setHeightForWidth(rowsListWidget->sizePolicy().hasHeightForWidth());
        rowsListWidget->setSizePolicy(sizePolicy1);
        rowsListWidget->setFocusPolicy(Qt::TabFocus);

        gridLayout_3->addWidget(rowsListWidget, 0, 0, 1, 1);

        horizontalLayout_4 = new QHBoxLayout();
        horizontalLayout_4->setObjectName(QStringLiteral("horizontalLayout_4"));
        newRowButton = new QToolButton(rowsBox);
        newRowButton->setObjectName(QStringLiteral("newRowButton"));

        horizontalLayout_4->addWidget(newRowButton);

        deleteRowButton = new QToolButton(rowsBox);
        deleteRowButton->setObjectName(QStringLiteral("deleteRowButton"));

        horizontalLayout_4->addWidget(deleteRowButton);

        spacerItem2 = new QSpacerItem(0, 23, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_4->addItem(spacerItem2);

        moveRowUpButton = new QToolButton(rowsBox);
        moveRowUpButton->setObjectName(QStringLiteral("moveRowUpButton"));

        horizontalLayout_4->addWidget(moveRowUpButton);

        moveRowDownButton = new QToolButton(rowsBox);
        moveRowDownButton->setObjectName(QStringLiteral("moveRowDownButton"));

        horizontalLayout_4->addWidget(moveRowDownButton);


        gridLayout_3->addLayout(horizontalLayout_4, 1, 0, 1, 1);

        horizontalLayout = new QHBoxLayout();
        horizontalLayout->setObjectName(QStringLiteral("horizontalLayout"));
        label_2 = new QLabel(rowsBox);
        label_2->setObjectName(QStringLiteral("label_2"));

        horizontalLayout->addWidget(label_2);

        rowIconSelector = new qdesigner_internal::IconSelector(rowsBox);
        rowIconSelector->setObjectName(QStringLiteral("rowIconSelector"));

        horizontalLayout->addWidget(rowIconSelector);

        spacerItem3 = new QSpacerItem(0, 21, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout->addItem(spacerItem3);


        gridLayout_3->addLayout(horizontalLayout, 2, 0, 1, 1);


        verticalLayout->addWidget(rowsBox);


        gridLayout_4->addWidget(widget, 0, 1, 1, 1);

        itemsBox->raise();
        buttonBox->raise();
        widget->raise();
        QWidget::setTabOrder(tableWidget, columnsListWidget);
        QWidget::setTabOrder(columnsListWidget, newColumnButton);
        QWidget::setTabOrder(newColumnButton, deleteColumnButton);
        QWidget::setTabOrder(deleteColumnButton, moveColumnUpButton);
        QWidget::setTabOrder(moveColumnUpButton, moveColumnDownButton);
        QWidget::setTabOrder(moveColumnDownButton, rowsListWidget);
        QWidget::setTabOrder(rowsListWidget, newRowButton);
        QWidget::setTabOrder(newRowButton, deleteRowButton);
        QWidget::setTabOrder(deleteRowButton, moveRowUpButton);
        QWidget::setTabOrder(moveRowUpButton, moveRowDownButton);

        retranslateUi(qdesigner_internal__TableWidgetEditor);
        QObject::connect(buttonBox, SIGNAL(accepted()), qdesigner_internal__TableWidgetEditor, SLOT(accept()));
        QObject::connect(buttonBox, SIGNAL(rejected()), qdesigner_internal__TableWidgetEditor, SLOT(reject()));

        QMetaObject::connectSlotsByName(qdesigner_internal__TableWidgetEditor);
    } // setupUi

    void retranslateUi(QDialog *qdesigner_internal__TableWidgetEditor)
    {
        qdesigner_internal__TableWidgetEditor->setWindowTitle(QApplication::translate("qdesigner_internal::TableWidgetEditor", "Edit Table Widget", nullptr));
        itemsBox->setTitle(QApplication::translate("qdesigner_internal::TableWidgetEditor", "Table Items", nullptr));
#ifndef QT_NO_TOOLTIP
        tableWidget->setToolTip(QApplication::translate("qdesigner_internal::TableWidgetEditor", "Table Items", nullptr));
#endif // QT_NO_TOOLTIP
        label_3->setText(QApplication::translate("qdesigner_internal::TableWidgetEditor", "Icon", nullptr));
        columnsBox->setTitle(QApplication::translate("qdesigner_internal::TableWidgetEditor", "Columns", nullptr));
#ifndef QT_NO_TOOLTIP
        columnsListWidget->setToolTip(QApplication::translate("qdesigner_internal::TableWidgetEditor", "Table Columns", nullptr));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_TOOLTIP
        newColumnButton->setToolTip(QApplication::translate("qdesigner_internal::TableWidgetEditor", "New Column", nullptr));
#endif // QT_NO_TOOLTIP
        newColumnButton->setText(QApplication::translate("qdesigner_internal::TableWidgetEditor", "New", nullptr));
#ifndef QT_NO_TOOLTIP
        deleteColumnButton->setToolTip(QApplication::translate("qdesigner_internal::TableWidgetEditor", "Delete Column", nullptr));
#endif // QT_NO_TOOLTIP
        deleteColumnButton->setText(QApplication::translate("qdesigner_internal::TableWidgetEditor", "Delete", nullptr));
#ifndef QT_NO_TOOLTIP
        moveColumnUpButton->setToolTip(QApplication::translate("qdesigner_internal::TableWidgetEditor", "Move Column Up", nullptr));
#endif // QT_NO_TOOLTIP
        moveColumnUpButton->setText(QApplication::translate("qdesigner_internal::TableWidgetEditor", "U", nullptr));
#ifndef QT_NO_TOOLTIP
        moveColumnDownButton->setToolTip(QApplication::translate("qdesigner_internal::TableWidgetEditor", "Move Column Down", nullptr));
#endif // QT_NO_TOOLTIP
        moveColumnDownButton->setText(QApplication::translate("qdesigner_internal::TableWidgetEditor", "D", nullptr));
        label->setText(QApplication::translate("qdesigner_internal::TableWidgetEditor", "Icon", nullptr));
        rowsBox->setTitle(QApplication::translate("qdesigner_internal::TableWidgetEditor", "Rows", nullptr));
#ifndef QT_NO_TOOLTIP
        rowsListWidget->setToolTip(QApplication::translate("qdesigner_internal::TableWidgetEditor", "Table Rows", nullptr));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_TOOLTIP
        newRowButton->setToolTip(QApplication::translate("qdesigner_internal::TableWidgetEditor", "New Row", nullptr));
#endif // QT_NO_TOOLTIP
        newRowButton->setText(QApplication::translate("qdesigner_internal::TableWidgetEditor", "New", nullptr));
#ifndef QT_NO_TOOLTIP
        deleteRowButton->setToolTip(QApplication::translate("qdesigner_internal::TableWidgetEditor", "Delete Row", nullptr));
#endif // QT_NO_TOOLTIP
        deleteRowButton->setText(QApplication::translate("qdesigner_internal::TableWidgetEditor", "Delete", nullptr));
#ifndef QT_NO_TOOLTIP
        moveRowUpButton->setToolTip(QApplication::translate("qdesigner_internal::TableWidgetEditor", "Move Row Up", nullptr));
#endif // QT_NO_TOOLTIP
        moveRowUpButton->setText(QApplication::translate("qdesigner_internal::TableWidgetEditor", "U", nullptr));
#ifndef QT_NO_TOOLTIP
        moveRowDownButton->setToolTip(QApplication::translate("qdesigner_internal::TableWidgetEditor", "Move Row Down", nullptr));
#endif // QT_NO_TOOLTIP
        moveRowDownButton->setText(QApplication::translate("qdesigner_internal::TableWidgetEditor", "D", nullptr));
        label_2->setText(QApplication::translate("qdesigner_internal::TableWidgetEditor", "Icon", nullptr));
    } // retranslateUi

};

} // namespace qdesigner_internal

namespace qdesigner_internal {
namespace Ui {
    class TableWidgetEditor: public Ui_TableWidgetEditor {};
} // namespace Ui
} // namespace qdesigner_internal

QT_END_NAMESPACE

#endif // TABLEWIDGETEDITOR_H

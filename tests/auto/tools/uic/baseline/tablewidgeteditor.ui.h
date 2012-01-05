/*
*********************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the autotests of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
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
** Form generated from reading UI file 'tablewidgeteditor.ui'
**
** Created: Fri Sep 4 10:17:15 2009
**      by: Qt User Interface Compiler version 4.6.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef TABLEWIDGETEDITOR_H
#define TABLEWIDGETEDITOR_H

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
            qdesigner_internal__TableWidgetEditor->setObjectName(QString::fromUtf8("qdesigner_internal__TableWidgetEditor"));
        qdesigner_internal__TableWidgetEditor->resize(591, 455);
        gridLayout_4 = new QGridLayout(qdesigner_internal__TableWidgetEditor);
        gridLayout_4->setObjectName(QString::fromUtf8("gridLayout_4"));
        itemsBox = new QGroupBox(qdesigner_internal__TableWidgetEditor);
        itemsBox->setObjectName(QString::fromUtf8("itemsBox"));
        gridLayout = new QGridLayout(itemsBox);
        gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
        tableWidget = new QTableWidget(itemsBox);
        tableWidget->setObjectName(QString::fromUtf8("tableWidget"));

        gridLayout->addWidget(tableWidget, 0, 0, 1, 1);

        horizontalLayout_5 = new QHBoxLayout();
        horizontalLayout_5->setObjectName(QString::fromUtf8("horizontalLayout_5"));
        label_3 = new QLabel(itemsBox);
        label_3->setObjectName(QString::fromUtf8("label_3"));

        horizontalLayout_5->addWidget(label_3);

        itemIconSelector = new qdesigner_internal::IconSelector(itemsBox);
        itemIconSelector->setObjectName(QString::fromUtf8("itemIconSelector"));

        horizontalLayout_5->addWidget(itemIconSelector);

        horizontalSpacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_5->addItem(horizontalSpacer);


        gridLayout->addLayout(horizontalLayout_5, 1, 0, 1, 1);


        gridLayout_4->addWidget(itemsBox, 0, 0, 1, 1);

        buttonBox = new QDialogButtonBox(qdesigner_internal__TableWidgetEditor);
        buttonBox->setObjectName(QString::fromUtf8("buttonBox"));
        buttonBox->setOrientation(Qt::Horizontal);
        buttonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);

        gridLayout_4->addWidget(buttonBox, 1, 0, 1, 2);

        widget = new QWidget(qdesigner_internal__TableWidgetEditor);
        widget->setObjectName(QString::fromUtf8("widget"));
        QSizePolicy sizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(widget->sizePolicy().hasHeightForWidth());
        widget->setSizePolicy(sizePolicy);
        verticalLayout = new QVBoxLayout(widget);
        verticalLayout->setContentsMargins(0, 0, 0, 0);
        verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
        columnsBox = new QGroupBox(widget);
        columnsBox->setObjectName(QString::fromUtf8("columnsBox"));
        gridLayout_2 = new QGridLayout(columnsBox);
        gridLayout_2->setObjectName(QString::fromUtf8("gridLayout_2"));
        columnsListWidget = new QListWidget(columnsBox);
        columnsListWidget->setObjectName(QString::fromUtf8("columnsListWidget"));
        QSizePolicy sizePolicy1(QSizePolicy::Ignored, QSizePolicy::Expanding);
        sizePolicy1.setHorizontalStretch(0);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(columnsListWidget->sizePolicy().hasHeightForWidth());
        columnsListWidget->setSizePolicy(sizePolicy1);
        columnsListWidget->setFocusPolicy(Qt::TabFocus);

        gridLayout_2->addWidget(columnsListWidget, 0, 0, 1, 1);

        horizontalLayout_3 = new QHBoxLayout();
        horizontalLayout_3->setObjectName(QString::fromUtf8("horizontalLayout_3"));
        newColumnButton = new QToolButton(columnsBox);
        newColumnButton->setObjectName(QString::fromUtf8("newColumnButton"));

        horizontalLayout_3->addWidget(newColumnButton);

        deleteColumnButton = new QToolButton(columnsBox);
        deleteColumnButton->setObjectName(QString::fromUtf8("deleteColumnButton"));

        horizontalLayout_3->addWidget(deleteColumnButton);

        spacerItem = new QSpacerItem(0, 23, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_3->addItem(spacerItem);

        moveColumnUpButton = new QToolButton(columnsBox);
        moveColumnUpButton->setObjectName(QString::fromUtf8("moveColumnUpButton"));

        horizontalLayout_3->addWidget(moveColumnUpButton);

        moveColumnDownButton = new QToolButton(columnsBox);
        moveColumnDownButton->setObjectName(QString::fromUtf8("moveColumnDownButton"));

        horizontalLayout_3->addWidget(moveColumnDownButton);


        gridLayout_2->addLayout(horizontalLayout_3, 1, 0, 1, 1);

        horizontalLayout_2 = new QHBoxLayout();
        horizontalLayout_2->setObjectName(QString::fromUtf8("horizontalLayout_2"));
        label = new QLabel(columnsBox);
        label->setObjectName(QString::fromUtf8("label"));

        horizontalLayout_2->addWidget(label);

        columnIconSelector = new qdesigner_internal::IconSelector(columnsBox);
        columnIconSelector->setObjectName(QString::fromUtf8("columnIconSelector"));

        horizontalLayout_2->addWidget(columnIconSelector);

        spacerItem1 = new QSpacerItem(0, 21, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_2->addItem(spacerItem1);


        gridLayout_2->addLayout(horizontalLayout_2, 2, 0, 1, 1);


        verticalLayout->addWidget(columnsBox);

        rowsBox = new QGroupBox(widget);
        rowsBox->setObjectName(QString::fromUtf8("rowsBox"));
        gridLayout_3 = new QGridLayout(rowsBox);
        gridLayout_3->setObjectName(QString::fromUtf8("gridLayout_3"));
        rowsListWidget = new QListWidget(rowsBox);
        rowsListWidget->setObjectName(QString::fromUtf8("rowsListWidget"));
        sizePolicy1.setHeightForWidth(rowsListWidget->sizePolicy().hasHeightForWidth());
        rowsListWidget->setSizePolicy(sizePolicy1);
        rowsListWidget->setFocusPolicy(Qt::TabFocus);

        gridLayout_3->addWidget(rowsListWidget, 0, 0, 1, 1);

        horizontalLayout_4 = new QHBoxLayout();
        horizontalLayout_4->setObjectName(QString::fromUtf8("horizontalLayout_4"));
        newRowButton = new QToolButton(rowsBox);
        newRowButton->setObjectName(QString::fromUtf8("newRowButton"));

        horizontalLayout_4->addWidget(newRowButton);

        deleteRowButton = new QToolButton(rowsBox);
        deleteRowButton->setObjectName(QString::fromUtf8("deleteRowButton"));

        horizontalLayout_4->addWidget(deleteRowButton);

        spacerItem2 = new QSpacerItem(0, 23, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_4->addItem(spacerItem2);

        moveRowUpButton = new QToolButton(rowsBox);
        moveRowUpButton->setObjectName(QString::fromUtf8("moveRowUpButton"));

        horizontalLayout_4->addWidget(moveRowUpButton);

        moveRowDownButton = new QToolButton(rowsBox);
        moveRowDownButton->setObjectName(QString::fromUtf8("moveRowDownButton"));

        horizontalLayout_4->addWidget(moveRowDownButton);


        gridLayout_3->addLayout(horizontalLayout_4, 1, 0, 1, 1);

        horizontalLayout = new QHBoxLayout();
        horizontalLayout->setObjectName(QString::fromUtf8("horizontalLayout"));
        label_2 = new QLabel(rowsBox);
        label_2->setObjectName(QString::fromUtf8("label_2"));

        horizontalLayout->addWidget(label_2);

        rowIconSelector = new qdesigner_internal::IconSelector(rowsBox);
        rowIconSelector->setObjectName(QString::fromUtf8("rowIconSelector"));

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
        qdesigner_internal__TableWidgetEditor->setWindowTitle(QApplication::translate("qdesigner_internal::TableWidgetEditor", "Edit Table Widget", 0, QApplication::UnicodeUTF8));
        itemsBox->setTitle(QApplication::translate("qdesigner_internal::TableWidgetEditor", "Table Items", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        tableWidget->setToolTip(QApplication::translate("qdesigner_internal::TableWidgetEditor", "Table Items", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        label_3->setText(QApplication::translate("qdesigner_internal::TableWidgetEditor", "Icon", 0, QApplication::UnicodeUTF8));
        columnsBox->setTitle(QApplication::translate("qdesigner_internal::TableWidgetEditor", "Columns", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        columnsListWidget->setToolTip(QApplication::translate("qdesigner_internal::TableWidgetEditor", "Table Columns", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_TOOLTIP
        newColumnButton->setToolTip(QApplication::translate("qdesigner_internal::TableWidgetEditor", "New Column", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        newColumnButton->setText(QApplication::translate("qdesigner_internal::TableWidgetEditor", "New", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        deleteColumnButton->setToolTip(QApplication::translate("qdesigner_internal::TableWidgetEditor", "Delete Column", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        deleteColumnButton->setText(QApplication::translate("qdesigner_internal::TableWidgetEditor", "Delete", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        moveColumnUpButton->setToolTip(QApplication::translate("qdesigner_internal::TableWidgetEditor", "Move Column Up", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        moveColumnUpButton->setText(QApplication::translate("qdesigner_internal::TableWidgetEditor", "U", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        moveColumnDownButton->setToolTip(QApplication::translate("qdesigner_internal::TableWidgetEditor", "Move Column Down", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        moveColumnDownButton->setText(QApplication::translate("qdesigner_internal::TableWidgetEditor", "D", 0, QApplication::UnicodeUTF8));
        label->setText(QApplication::translate("qdesigner_internal::TableWidgetEditor", "Icon", 0, QApplication::UnicodeUTF8));
        rowsBox->setTitle(QApplication::translate("qdesigner_internal::TableWidgetEditor", "Rows", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        rowsListWidget->setToolTip(QApplication::translate("qdesigner_internal::TableWidgetEditor", "Table Rows", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_TOOLTIP
        newRowButton->setToolTip(QApplication::translate("qdesigner_internal::TableWidgetEditor", "New Row", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        newRowButton->setText(QApplication::translate("qdesigner_internal::TableWidgetEditor", "New", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        deleteRowButton->setToolTip(QApplication::translate("qdesigner_internal::TableWidgetEditor", "Delete Row", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        deleteRowButton->setText(QApplication::translate("qdesigner_internal::TableWidgetEditor", "Delete", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        moveRowUpButton->setToolTip(QApplication::translate("qdesigner_internal::TableWidgetEditor", "Move Row Up", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        moveRowUpButton->setText(QApplication::translate("qdesigner_internal::TableWidgetEditor", "U", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        moveRowDownButton->setToolTip(QApplication::translate("qdesigner_internal::TableWidgetEditor", "Move Row Down", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        moveRowDownButton->setText(QApplication::translate("qdesigner_internal::TableWidgetEditor", "D", 0, QApplication::UnicodeUTF8));
        label_2->setText(QApplication::translate("qdesigner_internal::TableWidgetEditor", "Icon", 0, QApplication::UnicodeUTF8));
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

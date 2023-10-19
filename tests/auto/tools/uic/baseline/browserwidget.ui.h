/********************************************************************************
** Form generated from reading UI file 'browserwidget.ui'
**
** Created by: Qt User Interface Compiler version 6.0.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef BROWSERWIDGET_H
#define BROWSERWIDGET_H

#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QSplitter>
#include <QtWidgets/QTableView>
#include <QtWidgets/QTextEdit>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>
#include "connectionwidget.h"

QT_BEGIN_NAMESPACE

class Ui_Browser
{
public:
    QAction *insertRowAction;
    QAction *deleteRowAction;
    QVBoxLayout *vboxLayout;
    QSplitter *splitter_2;
    ConnectionWidget *connectionWidget;
    QTableView *table;
    QGroupBox *groupBox;
    QVBoxLayout *vboxLayout1;
    QTextEdit *sqlEdit;
    QHBoxLayout *hboxLayout;
    QSpacerItem *spacerItem;
    QPushButton *clearButton;
    QPushButton *submitButton;

    void setupUi(QWidget *Browser)
    {
        if (Browser->objectName().isEmpty())
            Browser->setObjectName("Browser");
        Browser->resize(765, 515);
        insertRowAction = new QAction(Browser);
        insertRowAction->setObjectName("insertRowAction");
        insertRowAction->setEnabled(false);
        deleteRowAction = new QAction(Browser);
        deleteRowAction->setObjectName("deleteRowAction");
        deleteRowAction->setEnabled(false);
        vboxLayout = new QVBoxLayout(Browser);
#ifndef Q_OS_MAC
        vboxLayout->setSpacing(6);
#endif
        vboxLayout->setContentsMargins(8, 8, 8, 8);
        vboxLayout->setObjectName("vboxLayout");
        splitter_2 = new QSplitter(Browser);
        splitter_2->setObjectName("splitter_2");
        QSizePolicy sizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Expanding);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(splitter_2->sizePolicy().hasHeightForWidth());
        splitter_2->setSizePolicy(sizePolicy);
        splitter_2->setOrientation(Qt::Horizontal);
        connectionWidget = new ConnectionWidget(splitter_2);
        connectionWidget->setObjectName("connectionWidget");
        QSizePolicy sizePolicy1(QSizePolicy::Policy::Ignored, QSizePolicy::Policy::Expanding);
        sizePolicy1.setHorizontalStretch(1);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(connectionWidget->sizePolicy().hasHeightForWidth());
        connectionWidget->setSizePolicy(sizePolicy1);
        splitter_2->addWidget(connectionWidget);
        table = new QTableView(splitter_2);
        table->setObjectName("table");
        QSizePolicy sizePolicy2(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Expanding);
        sizePolicy2.setHorizontalStretch(2);
        sizePolicy2.setVerticalStretch(0);
        sizePolicy2.setHeightForWidth(table->sizePolicy().hasHeightForWidth());
        table->setSizePolicy(sizePolicy2);
        table->setContextMenuPolicy(Qt::ActionsContextMenu);
        table->setSelectionBehavior(QAbstractItemView::SelectRows);
        splitter_2->addWidget(table);

        vboxLayout->addWidget(splitter_2);

        groupBox = new QGroupBox(Browser);
        groupBox->setObjectName("groupBox");
        QSizePolicy sizePolicy3(QSizePolicy::Policy::Preferred, QSizePolicy::Policy::MinimumExpanding);
        sizePolicy3.setHorizontalStretch(0);
        sizePolicy3.setVerticalStretch(0);
        sizePolicy3.setHeightForWidth(groupBox->sizePolicy().hasHeightForWidth());
        groupBox->setSizePolicy(sizePolicy3);
        groupBox->setMaximumSize(QSize(16777215, 180));
        vboxLayout1 = new QVBoxLayout(groupBox);
#ifndef Q_OS_MAC
        vboxLayout1->setSpacing(6);
#endif
#ifndef Q_OS_MAC
        vboxLayout1->setContentsMargins(9, 9, 9, 9);
#endif
        vboxLayout1->setObjectName("vboxLayout1");
        sqlEdit = new QTextEdit(groupBox);
        sqlEdit->setObjectName("sqlEdit");
        QSizePolicy sizePolicy4(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::MinimumExpanding);
        sizePolicy4.setHorizontalStretch(0);
        sizePolicy4.setVerticalStretch(0);
        sizePolicy4.setHeightForWidth(sqlEdit->sizePolicy().hasHeightForWidth());
        sqlEdit->setSizePolicy(sizePolicy4);
        sqlEdit->setMinimumSize(QSize(0, 18));
        sqlEdit->setBaseSize(QSize(0, 120));

        vboxLayout1->addWidget(sqlEdit);

        hboxLayout = new QHBoxLayout();
#ifndef Q_OS_MAC
        hboxLayout->setSpacing(6);
#endif
        hboxLayout->setContentsMargins(1, 1, 1, 1);
        hboxLayout->setObjectName("hboxLayout");
        spacerItem = new QSpacerItem(40, 20, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        hboxLayout->addItem(spacerItem);

        clearButton = new QPushButton(groupBox);
        clearButton->setObjectName("clearButton");

        hboxLayout->addWidget(clearButton);

        submitButton = new QPushButton(groupBox);
        submitButton->setObjectName("submitButton");

        hboxLayout->addWidget(submitButton);


        vboxLayout1->addLayout(hboxLayout);


        vboxLayout->addWidget(groupBox);

        QWidget::setTabOrder(sqlEdit, clearButton);
        QWidget::setTabOrder(clearButton, submitButton);
        QWidget::setTabOrder(submitButton, connectionWidget);
        QWidget::setTabOrder(connectionWidget, table);

        retranslateUi(Browser);

        QMetaObject::connectSlotsByName(Browser);
    } // setupUi

    void retranslateUi(QWidget *Browser)
    {
        Browser->setWindowTitle(QCoreApplication::translate("Browser", "Qt SQL Browser", nullptr));
        insertRowAction->setText(QCoreApplication::translate("Browser", "&Insert Row", nullptr));
#if QT_CONFIG(statustip)
        insertRowAction->setStatusTip(QCoreApplication::translate("Browser", "Inserts a new Row", nullptr));
#endif // QT_CONFIG(statustip)
        deleteRowAction->setText(QCoreApplication::translate("Browser", "&Delete Row", nullptr));
#if QT_CONFIG(statustip)
        deleteRowAction->setStatusTip(QCoreApplication::translate("Browser", "Deletes the current Row", nullptr));
#endif // QT_CONFIG(statustip)
        groupBox->setTitle(QCoreApplication::translate("Browser", "SQL Query", nullptr));
        clearButton->setText(QCoreApplication::translate("Browser", "&Clear", nullptr));
        submitButton->setText(QCoreApplication::translate("Browser", "&Submit", nullptr));
    } // retranslateUi

};

namespace Ui {
    class Browser: public Ui_Browser {};
} // namespace Ui

QT_END_NAMESPACE

#endif // BROWSERWIDGET_H

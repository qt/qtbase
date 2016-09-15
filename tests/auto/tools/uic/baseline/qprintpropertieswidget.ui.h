/********************************************************************************
** Form generated from reading UI file 'qprintpropertieswidget.ui'
**
** Created by: Qt User Interface Compiler version 5.0.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef QPRINTPROPERTIESWIDGET_H
#define QPRINTPROPERTIESWIDGET_H

#include <QtCore/QVariant>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QTreeView>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>
#include "qpagesetupdialog_unix_p.h"

QT_BEGIN_NAMESPACE

class Ui_QPrintPropertiesWidget
{
public:
    QVBoxLayout *verticalLayout_4;
    QTabWidget *tabs;
    QWidget *tabPage;
    QHBoxLayout *horizontalLayout;
    QPageSetupWidget *pageSetup;
    QWidget *cupsPropertiesPage;
    QHBoxLayout *horizontalLayout_2;
    QTreeView *treeView;

    void setupUi(QWidget *QPrintPropertiesWidget)
    {
        if (QPrintPropertiesWidget->objectName().isEmpty())
            QPrintPropertiesWidget->setObjectName(QStringLiteral("QPrintPropertiesWidget"));
        QPrintPropertiesWidget->resize(396, 288);
        verticalLayout_4 = new QVBoxLayout(QPrintPropertiesWidget);
        verticalLayout_4->setContentsMargins(0, 0, 0, 0);
        verticalLayout_4->setObjectName(QStringLiteral("verticalLayout_4"));
        tabs = new QTabWidget(QPrintPropertiesWidget);
        tabs->setObjectName(QStringLiteral("tabs"));
        tabPage = new QWidget();
        tabPage->setObjectName(QStringLiteral("tabPage"));
        tabPage->setGeometry(QRect(0, 0, 392, 261));
        horizontalLayout = new QHBoxLayout(tabPage);
        horizontalLayout->setObjectName(QStringLiteral("horizontalLayout"));
        pageSetup = new QPageSetupWidget(tabPage);
        pageSetup->setObjectName(QStringLiteral("pageSetup"));

        horizontalLayout->addWidget(pageSetup);

        tabs->addTab(tabPage, QString());
        cupsPropertiesPage = new QWidget();
        cupsPropertiesPage->setObjectName(QStringLiteral("cupsPropertiesPage"));
        horizontalLayout_2 = new QHBoxLayout(cupsPropertiesPage);
        horizontalLayout_2->setObjectName(QStringLiteral("horizontalLayout_2"));
        treeView = new QTreeView(cupsPropertiesPage);
        treeView->setObjectName(QStringLiteral("treeView"));
        treeView->setAlternatingRowColors(true);

        horizontalLayout_2->addWidget(treeView);

        tabs->addTab(cupsPropertiesPage, QString());

        verticalLayout_4->addWidget(tabs);


        retranslateUi(QPrintPropertiesWidget);

        tabs->setCurrentIndex(0);


        QMetaObject::connectSlotsByName(QPrintPropertiesWidget);
    } // setupUi

    void retranslateUi(QWidget *QPrintPropertiesWidget)
    {
        QPrintPropertiesWidget->setWindowTitle(QApplication::translate("QPrintPropertiesWidget", "Form", Q_NULLPTR));
        tabs->setTabText(tabs->indexOf(tabPage), QApplication::translate("QPrintPropertiesWidget", "Page", Q_NULLPTR));
        tabs->setTabText(tabs->indexOf(cupsPropertiesPage), QApplication::translate("QPrintPropertiesWidget", "Advanced", Q_NULLPTR));
    } // retranslateUi

};

namespace Ui {
    class QPrintPropertiesWidget: public Ui_QPrintPropertiesWidget {};
} // namespace Ui

QT_END_NAMESPACE

#endif // QPRINTPROPERTIESWIDGET_H

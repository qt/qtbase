/********************************************************************************
** Form generated from reading UI file 'backside.ui'
**
** Created by: Qt User Interface Compiler version 5.12.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef BACKSIDE_H
#define BACKSIDE_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QDateTimeEdit>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QSlider>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QTreeWidget>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_BackSide
{
public:
    QVBoxLayout *verticalLayout_2;
    QGroupBox *groupBox;
    QGridLayout *gridLayout;
    QLabel *label;
    QLineEdit *hostName;
    QLabel *label_2;
    QLabel *label_3;
    QHBoxLayout *horizontalLayout;
    QSlider *horizontalSlider;
    QSpinBox *spinBox;
    QDateTimeEdit *dateTimeEdit;
    QGroupBox *groupBox_2;
    QHBoxLayout *horizontalLayout_2;
    QTreeWidget *treeWidget;

    void setupUi(QWidget *BackSide)
    {
        if (BackSide->objectName().isEmpty())
            BackSide->setObjectName(QString::fromUtf8("BackSide"));
        BackSide->resize(378, 385);
        verticalLayout_2 = new QVBoxLayout(BackSide);
        verticalLayout_2->setObjectName(QString::fromUtf8("verticalLayout_2"));
        groupBox = new QGroupBox(BackSide);
        groupBox->setObjectName(QString::fromUtf8("groupBox"));
        groupBox->setFlat(true);
        groupBox->setCheckable(true);
        gridLayout = new QGridLayout(groupBox);
        gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
        label = new QLabel(groupBox);
        label->setObjectName(QString::fromUtf8("label"));

        gridLayout->addWidget(label, 0, 0, 1, 1);

        hostName = new QLineEdit(groupBox);
        hostName->setObjectName(QString::fromUtf8("hostName"));

        gridLayout->addWidget(hostName, 0, 1, 1, 1);

        label_2 = new QLabel(groupBox);
        label_2->setObjectName(QString::fromUtf8("label_2"));

        gridLayout->addWidget(label_2, 1, 0, 1, 1);

        label_3 = new QLabel(groupBox);
        label_3->setObjectName(QString::fromUtf8("label_3"));

        gridLayout->addWidget(label_3, 2, 0, 1, 1);

        horizontalLayout = new QHBoxLayout();
        horizontalLayout->setObjectName(QString::fromUtf8("horizontalLayout"));
        horizontalSlider = new QSlider(groupBox);
        horizontalSlider->setObjectName(QString::fromUtf8("horizontalSlider"));
        horizontalSlider->setValue(42);
        horizontalSlider->setOrientation(Qt::Horizontal);

        horizontalLayout->addWidget(horizontalSlider);

        spinBox = new QSpinBox(groupBox);
        spinBox->setObjectName(QString::fromUtf8("spinBox"));
        spinBox->setValue(42);

        horizontalLayout->addWidget(spinBox);


        gridLayout->addLayout(horizontalLayout, 2, 1, 1, 1);

        dateTimeEdit = new QDateTimeEdit(groupBox);
        dateTimeEdit->setObjectName(QString::fromUtf8("dateTimeEdit"));

        gridLayout->addWidget(dateTimeEdit, 1, 1, 1, 1);


        verticalLayout_2->addWidget(groupBox);

        groupBox_2 = new QGroupBox(BackSide);
        groupBox_2->setObjectName(QString::fromUtf8("groupBox_2"));
        groupBox_2->setFlat(true);
        groupBox_2->setCheckable(true);
        horizontalLayout_2 = new QHBoxLayout(groupBox_2);
        horizontalLayout_2->setObjectName(QString::fromUtf8("horizontalLayout_2"));
        treeWidget = new QTreeWidget(groupBox_2);
        QTreeWidgetItem *__qtreewidgetitem = new QTreeWidgetItem(treeWidget);
        QTreeWidgetItem *__qtreewidgetitem1 = new QTreeWidgetItem(__qtreewidgetitem);
        new QTreeWidgetItem(__qtreewidgetitem1);
        new QTreeWidgetItem(__qtreewidgetitem1);
        new QTreeWidgetItem(__qtreewidgetitem1);
        QTreeWidgetItem *__qtreewidgetitem2 = new QTreeWidgetItem(treeWidget);
        QTreeWidgetItem *__qtreewidgetitem3 = new QTreeWidgetItem(__qtreewidgetitem2);
        new QTreeWidgetItem(__qtreewidgetitem3);
        QTreeWidgetItem *__qtreewidgetitem4 = new QTreeWidgetItem(treeWidget);
        QTreeWidgetItem *__qtreewidgetitem5 = new QTreeWidgetItem(__qtreewidgetitem4);
        new QTreeWidgetItem(__qtreewidgetitem5);
        treeWidget->setObjectName(QString::fromUtf8("treeWidget"));

        horizontalLayout_2->addWidget(treeWidget);


        verticalLayout_2->addWidget(groupBox_2);

        QWidget::setTabOrder(groupBox, hostName);
        QWidget::setTabOrder(hostName, dateTimeEdit);
        QWidget::setTabOrder(dateTimeEdit, horizontalSlider);
        QWidget::setTabOrder(horizontalSlider, spinBox);
        QWidget::setTabOrder(spinBox, groupBox_2);
        QWidget::setTabOrder(groupBox_2, treeWidget);

        retranslateUi(BackSide);
        QObject::connect(horizontalSlider, SIGNAL(valueChanged(int)), spinBox, SLOT(setValue(int)));
        QObject::connect(spinBox, SIGNAL(valueChanged(int)), horizontalSlider, SLOT(setValue(int)));

        QMetaObject::connectSlotsByName(BackSide);
    } // setupUi

    void retranslateUi(QWidget *BackSide)
    {
        BackSide->setWindowTitle(QCoreApplication::translate("BackSide", "BackSide", nullptr));
        groupBox->setTitle(QCoreApplication::translate("BackSide", "Settings", nullptr));
        label->setText(QCoreApplication::translate("BackSide", "Title:", nullptr));
        hostName->setText(QCoreApplication::translate("BackSide", "Pad Navigator Example", nullptr));
        label_2->setText(QCoreApplication::translate("BackSide", "Modified:", nullptr));
        label_3->setText(QCoreApplication::translate("BackSide", "Extent", nullptr));
        groupBox_2->setTitle(QCoreApplication::translate("BackSide", "Other input", nullptr));
        QTreeWidgetItem *___qtreewidgetitem = treeWidget->headerItem();
        ___qtreewidgetitem->setText(0, QCoreApplication::translate("BackSide", "Widgets On Graphics View", nullptr));

        const bool __sortingEnabled = treeWidget->isSortingEnabled();
        treeWidget->setSortingEnabled(false);
        QTreeWidgetItem *___qtreewidgetitem1 = treeWidget->topLevelItem(0);
        ___qtreewidgetitem1->setText(0, QCoreApplication::translate("BackSide", "QGraphicsProxyWidget", nullptr));
        QTreeWidgetItem *___qtreewidgetitem2 = ___qtreewidgetitem1->child(0);
        ___qtreewidgetitem2->setText(0, QCoreApplication::translate("BackSide", "QGraphicsWidget", nullptr));
        QTreeWidgetItem *___qtreewidgetitem3 = ___qtreewidgetitem2->child(0);
        ___qtreewidgetitem3->setText(0, QCoreApplication::translate("BackSide", "QObject", nullptr));
        QTreeWidgetItem *___qtreewidgetitem4 = ___qtreewidgetitem2->child(1);
        ___qtreewidgetitem4->setText(0, QCoreApplication::translate("BackSide", "QGraphicsItem", nullptr));
        QTreeWidgetItem *___qtreewidgetitem5 = ___qtreewidgetitem2->child(2);
        ___qtreewidgetitem5->setText(0, QCoreApplication::translate("BackSide", "QGraphicsLayoutItem", nullptr));
        QTreeWidgetItem *___qtreewidgetitem6 = treeWidget->topLevelItem(1);
        ___qtreewidgetitem6->setText(0, QCoreApplication::translate("BackSide", "QGraphicsGridLayout", nullptr));
        QTreeWidgetItem *___qtreewidgetitem7 = ___qtreewidgetitem6->child(0);
        ___qtreewidgetitem7->setText(0, QCoreApplication::translate("BackSide", "QGraphicsLayout", nullptr));
        QTreeWidgetItem *___qtreewidgetitem8 = ___qtreewidgetitem7->child(0);
        ___qtreewidgetitem8->setText(0, QCoreApplication::translate("BackSide", "QGraphicsLayoutItem", nullptr));
        QTreeWidgetItem *___qtreewidgetitem9 = treeWidget->topLevelItem(2);
        ___qtreewidgetitem9->setText(0, QCoreApplication::translate("BackSide", "QGraphicsLinearLayout", nullptr));
        QTreeWidgetItem *___qtreewidgetitem10 = ___qtreewidgetitem9->child(0);
        ___qtreewidgetitem10->setText(0, QCoreApplication::translate("BackSide", "QGraphicsLayout", nullptr));
        QTreeWidgetItem *___qtreewidgetitem11 = ___qtreewidgetitem10->child(0);
        ___qtreewidgetitem11->setText(0, QCoreApplication::translate("BackSide", "QGraphicsLayoutItem", nullptr));
        treeWidget->setSortingEnabled(__sortingEnabled);

    } // retranslateUi

};

namespace Ui {
    class BackSide: public Ui_BackSide {};
} // namespace Ui

QT_END_NAMESPACE

#endif // BACKSIDE_H

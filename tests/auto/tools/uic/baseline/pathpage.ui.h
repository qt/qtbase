/********************************************************************************
** Form generated from reading UI file 'pathpage.ui'
**
** Created by: Qt User Interface Compiler version 5.12.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef PATHPAGE_H
#define PATHPAGE_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_PathPage
{
public:
    QGridLayout *gridLayout;
    QLabel *label_2;
    QLineEdit *filterLineEdit;
    QSpacerItem *spacerItem;
    QLabel *label;
    QListWidget *pathListWidget;
    QPushButton *addButton;
    QPushButton *removeButton;
    QSpacerItem *spacerItem1;
    QSpacerItem *spacerItem2;

    void setupUi(QWidget *PathPage)
    {
        if (PathPage->objectName().isEmpty())
            PathPage->setObjectName(QString::fromUtf8("PathPage"));
        PathPage->resize(417, 243);
        gridLayout = new QGridLayout(PathPage);
        gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
        label_2 = new QLabel(PathPage);
        label_2->setObjectName(QString::fromUtf8("label_2"));
        QSizePolicy sizePolicy(QSizePolicy::Maximum, QSizePolicy::Preferred);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(label_2->sizePolicy().hasHeightForWidth());
        label_2->setSizePolicy(sizePolicy);

        gridLayout->addWidget(label_2, 0, 0, 1, 1);

        filterLineEdit = new QLineEdit(PathPage);
        filterLineEdit->setObjectName(QString::fromUtf8("filterLineEdit"));

        gridLayout->addWidget(filterLineEdit, 0, 1, 1, 2);

        spacerItem = new QSpacerItem(20, 10, QSizePolicy::Minimum, QSizePolicy::Fixed);

        gridLayout->addItem(spacerItem, 1, 1, 1, 1);

        label = new QLabel(PathPage);
        label->setObjectName(QString::fromUtf8("label"));

        gridLayout->addWidget(label, 2, 0, 1, 3);

        pathListWidget = new QListWidget(PathPage);
        pathListWidget->setObjectName(QString::fromUtf8("pathListWidget"));

        gridLayout->addWidget(pathListWidget, 3, 0, 3, 3);

        addButton = new QPushButton(PathPage);
        addButton->setObjectName(QString::fromUtf8("addButton"));
        QSizePolicy sizePolicy1(QSizePolicy::Maximum, QSizePolicy::Fixed);
        sizePolicy1.setHorizontalStretch(0);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(addButton->sizePolicy().hasHeightForWidth());
        addButton->setSizePolicy(sizePolicy1);

        gridLayout->addWidget(addButton, 3, 3, 1, 1);

        removeButton = new QPushButton(PathPage);
        removeButton->setObjectName(QString::fromUtf8("removeButton"));
        sizePolicy1.setHeightForWidth(removeButton->sizePolicy().hasHeightForWidth());
        removeButton->setSizePolicy(sizePolicy1);

        gridLayout->addWidget(removeButton, 4, 3, 1, 1);

        spacerItem1 = new QSpacerItem(20, 51, QSizePolicy::Minimum, QSizePolicy::Expanding);

        gridLayout->addItem(spacerItem1, 5, 3, 1, 1);

        spacerItem2 = new QSpacerItem(20, 31, QSizePolicy::Minimum, QSizePolicy::Expanding);

        gridLayout->addItem(spacerItem2, 6, 2, 1, 1);


        retranslateUi(PathPage);

        QMetaObject::connectSlotsByName(PathPage);
    } // setupUi

    void retranslateUi(QWidget *PathPage)
    {
        PathPage->setWindowTitle(QCoreApplication::translate("PathPage", "Form", nullptr));
        label_2->setText(QCoreApplication::translate("PathPage", "File filters:", nullptr));
        label->setText(QCoreApplication::translate("PathPage", "Documentation source file paths:", nullptr));
        addButton->setText(QCoreApplication::translate("PathPage", "Add", nullptr));
        removeButton->setText(QCoreApplication::translate("PathPage", "Remove", nullptr));
    } // retranslateUi

};

namespace Ui {
    class PathPage: public Ui_PathPage {};
} // namespace Ui

QT_END_NAMESPACE

#endif // PATHPAGE_H

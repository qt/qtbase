/********************************************************************************
** Form generated from reading UI file 'generalpage.ui'
**
** Created: Fri Sep 4 10:17:13 2009
**      by: Qt User Interface Compiler version 4.6.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef GENERALPAGE_H
#define GENERALPAGE_H

#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QGridLayout>
#include <QtGui/QHeaderView>
#include <QtGui/QLabel>
#include <QtGui/QLineEdit>
#include <QtGui/QSpacerItem>
#include <QtGui/QWidget>

QT_BEGIN_NAMESPACE

class Ui_GeneralPage
{
public:
    QGridLayout *gridLayout;
    QLabel *label;
    QLineEdit *namespaceLineEdit;
    QLabel *label_2;
    QLineEdit *folderLineEdit;
    QSpacerItem *spacerItem;
    QSpacerItem *spacerItem1;

    void setupUi(QWidget *GeneralPage)
    {
        if (GeneralPage->objectName().isEmpty())
            GeneralPage->setObjectName(QString::fromUtf8("GeneralPage"));
        GeneralPage->resize(417, 243);
        gridLayout = new QGridLayout(GeneralPage);
        gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
        label = new QLabel(GeneralPage);
        label->setObjectName(QString::fromUtf8("label"));

        gridLayout->addWidget(label, 1, 0, 1, 1);

        namespaceLineEdit = new QLineEdit(GeneralPage);
        namespaceLineEdit->setObjectName(QString::fromUtf8("namespaceLineEdit"));

        gridLayout->addWidget(namespaceLineEdit, 1, 1, 1, 1);

        label_2 = new QLabel(GeneralPage);
        label_2->setObjectName(QString::fromUtf8("label_2"));

        gridLayout->addWidget(label_2, 2, 0, 1, 1);

        folderLineEdit = new QLineEdit(GeneralPage);
        folderLineEdit->setObjectName(QString::fromUtf8("folderLineEdit"));

        gridLayout->addWidget(folderLineEdit, 2, 1, 1, 1);

        spacerItem = new QSpacerItem(20, 20, QSizePolicy::Minimum, QSizePolicy::Fixed);

        gridLayout->addItem(spacerItem, 0, 1, 1, 1);

        spacerItem1 = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

        gridLayout->addItem(spacerItem1, 3, 1, 1, 1);


        retranslateUi(GeneralPage);

        QMetaObject::connectSlotsByName(GeneralPage);
    } // setupUi

    void retranslateUi(QWidget *GeneralPage)
    {
        GeneralPage->setWindowTitle(QApplication::translate("GeneralPage", "Form", 0, QApplication::UnicodeUTF8));
        label->setText(QApplication::translate("GeneralPage", "Namespace:", 0, QApplication::UnicodeUTF8));
        label_2->setText(QApplication::translate("GeneralPage", "Virtual Folder:", 0, QApplication::UnicodeUTF8));
    } // retranslateUi

};

namespace Ui {
    class GeneralPage: public Ui_GeneralPage {};
} // namespace Ui

QT_END_NAMESPACE

#endif // GENERALPAGE_H

/********************************************************************************
** Form generated from reading UI file 'identifierpage.ui'
**
** Created by: Qt User Interface Compiler version 5.0.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef IDENTIFIERPAGE_H
#define IDENTIFIERPAGE_H

#include <QtCore/QVariant>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QRadioButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_IdentifierPage
{
public:
    QGridLayout *gridLayout;
    QSpacerItem *spacerItem;
    QCheckBox *identifierCheckBox;
    QSpacerItem *spacerItem1;
    QSpacerItem *spacerItem2;
    QRadioButton *globalButton;
    QLineEdit *prefixLineEdit;
    QRadioButton *fileNameButton;
    QSpacerItem *spacerItem3;

    void setupUi(QWidget *IdentifierPage)
    {
        if (IdentifierPage->objectName().isEmpty())
            IdentifierPage->setObjectName(QStringLiteral("IdentifierPage"));
        IdentifierPage->resize(417, 242);
        gridLayout = new QGridLayout(IdentifierPage);
        gridLayout->setObjectName(QStringLiteral("gridLayout"));
        spacerItem = new QSpacerItem(20, 20, QSizePolicy::Minimum, QSizePolicy::Fixed);

        gridLayout->addItem(spacerItem, 0, 1, 1, 1);

        identifierCheckBox = new QCheckBox(IdentifierPage);
        identifierCheckBox->setObjectName(QStringLiteral("identifierCheckBox"));

        gridLayout->addWidget(identifierCheckBox, 1, 0, 1, 3);

        spacerItem1 = new QSpacerItem(161, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        gridLayout->addItem(spacerItem1, 1, 3, 1, 1);

        spacerItem2 = new QSpacerItem(30, 20, QSizePolicy::Fixed, QSizePolicy::Minimum);

        gridLayout->addItem(spacerItem2, 2, 0, 1, 1);

        globalButton = new QRadioButton(IdentifierPage);
        globalButton->setObjectName(QStringLiteral("globalButton"));
        globalButton->setEnabled(false);
        globalButton->setChecked(true);

        gridLayout->addWidget(globalButton, 2, 1, 1, 1);

        prefixLineEdit = new QLineEdit(IdentifierPage);
        prefixLineEdit->setObjectName(QStringLiteral("prefixLineEdit"));
        prefixLineEdit->setEnabled(false);

        gridLayout->addWidget(prefixLineEdit, 2, 2, 1, 1);

        fileNameButton = new QRadioButton(IdentifierPage);
        fileNameButton->setObjectName(QStringLiteral("fileNameButton"));
        fileNameButton->setEnabled(false);

        gridLayout->addWidget(fileNameButton, 3, 1, 1, 2);

        spacerItem3 = new QSpacerItem(20, 31, QSizePolicy::Minimum, QSizePolicy::Expanding);

        gridLayout->addItem(spacerItem3, 4, 1, 1, 1);


        retranslateUi(IdentifierPage);
        QObject::connect(globalButton, SIGNAL(toggled(bool)), prefixLineEdit, SLOT(setEnabled(bool)));

        QMetaObject::connectSlotsByName(IdentifierPage);
    } // setupUi

    void retranslateUi(QWidget *IdentifierPage)
    {
        IdentifierPage->setWindowTitle(QApplication::translate("IdentifierPage", "Form", 0));
        identifierCheckBox->setText(QApplication::translate("IdentifierPage", "Create identifiers", 0));
        globalButton->setText(QApplication::translate("IdentifierPage", "Global prefix:", 0));
        fileNameButton->setText(QApplication::translate("IdentifierPage", "Inherit prefix from file names", 0));
    } // retranslateUi

};

namespace Ui {
    class IdentifierPage: public Ui_IdentifierPage {};
} // namespace Ui

QT_END_NAMESPACE

#endif // IDENTIFIERPAGE_H

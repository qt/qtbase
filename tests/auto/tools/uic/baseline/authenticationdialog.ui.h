/********************************************************************************
** Form generated from reading UI file 'authenticationdialog.ui'
**
** Created by: Qt User Interface Compiler version 5.0.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef AUTHENTICATIONDIALOG_H
#define AUTHENTICATIONDIALOG_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QDialog>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QSpacerItem>

QT_BEGIN_NAMESPACE

class Ui_Dialog
{
public:
    QGridLayout *gridLayout;
    QLabel *label;
    QLabel *label_2;
    QLineEdit *userEdit;
    QLabel *label_3;
    QLineEdit *passwordEdit;
    QDialogButtonBox *buttonBox;
    QLabel *label_4;
    QLabel *siteDescription;
    QSpacerItem *spacerItem;

    void setupUi(QDialog *Dialog)
    {
        if (Dialog->objectName().isEmpty())
            Dialog->setObjectName(QStringLiteral("Dialog"));
        Dialog->resize(389, 243);
        gridLayout = new QGridLayout(Dialog);
        gridLayout->setObjectName(QStringLiteral("gridLayout"));
        label = new QLabel(Dialog);
        label->setObjectName(QStringLiteral("label"));
        label->setWordWrap(false);

        gridLayout->addWidget(label, 0, 0, 1, 2);

        label_2 = new QLabel(Dialog);
        label_2->setObjectName(QStringLiteral("label_2"));

        gridLayout->addWidget(label_2, 2, 0, 1, 1);

        userEdit = new QLineEdit(Dialog);
        userEdit->setObjectName(QStringLiteral("userEdit"));

        gridLayout->addWidget(userEdit, 2, 1, 1, 1);

        label_3 = new QLabel(Dialog);
        label_3->setObjectName(QStringLiteral("label_3"));

        gridLayout->addWidget(label_3, 3, 0, 1, 1);

        passwordEdit = new QLineEdit(Dialog);
        passwordEdit->setObjectName(QStringLiteral("passwordEdit"));

        gridLayout->addWidget(passwordEdit, 3, 1, 1, 1);

        buttonBox = new QDialogButtonBox(Dialog);
        buttonBox->setObjectName(QStringLiteral("buttonBox"));
        buttonBox->setOrientation(Qt::Horizontal);
        buttonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);

        gridLayout->addWidget(buttonBox, 5, 0, 1, 2);

        label_4 = new QLabel(Dialog);
        label_4->setObjectName(QStringLiteral("label_4"));

        gridLayout->addWidget(label_4, 1, 0, 1, 1);

        siteDescription = new QLabel(Dialog);
        siteDescription->setObjectName(QStringLiteral("siteDescription"));
        QFont font;
        font.setBold(true);
        font.setWeight(75);
        siteDescription->setFont(font);
        siteDescription->setWordWrap(true);

        gridLayout->addWidget(siteDescription, 1, 1, 1, 1);

        spacerItem = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

        gridLayout->addItem(spacerItem, 4, 0, 1, 1);


        retranslateUi(Dialog);
        QObject::connect(buttonBox, SIGNAL(accepted()), Dialog, SLOT(accept()));
        QObject::connect(buttonBox, SIGNAL(rejected()), Dialog, SLOT(reject()));

        QMetaObject::connectSlotsByName(Dialog);
    } // setupUi

    void retranslateUi(QDialog *Dialog)
    {
        Dialog->setWindowTitle(QApplication::translate("Dialog", "Http authentication required", nullptr));
        label->setText(QApplication::translate("Dialog", "You need to supply a Username and a Password to access this site", nullptr));
        label_2->setText(QApplication::translate("Dialog", "Username:", nullptr));
        label_3->setText(QApplication::translate("Dialog", "Password:", nullptr));
        label_4->setText(QApplication::translate("Dialog", "Site:", nullptr));
        siteDescription->setText(QApplication::translate("Dialog", "%1 at %2", nullptr));
    } // retranslateUi

};

namespace Ui {
    class Dialog: public Ui_Dialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // AUTHENTICATIONDIALOG_H

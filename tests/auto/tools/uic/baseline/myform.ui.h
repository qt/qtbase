/********************************************************************************
** Form generated from reading UI file 'myform.ui'
**
** Created by: Qt User Interface Compiler version 5.12.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef MYFORM_H
#define MYFORM_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QRadioButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_Form
{
public:
    QVBoxLayout *vboxLayout;
    QGroupBox *groupBox;
    QGridLayout *gridLayout;
    QRadioButton *radioButton_2;
    QRadioButton *radioButton;
    QCheckBox *checkBox_2;
    QSpacerItem *spacerItem;
    QCheckBox *checkBox;
    QRadioButton *radioButton_2_2;
    QRadioButton *radioButton_3;
    QRadioButton *radioButton_4;
    QCheckBox *checkBox_3;
    QCheckBox *checkBox_4;

    void setupUi(QWidget *Form)
    {
        if (Form->objectName().isEmpty())
            Form->setObjectName(QString::fromUtf8("Form"));
        Form->resize(258, 224);
        vboxLayout = new QVBoxLayout(Form);
#ifndef Q_OS_MAC
        vboxLayout->setSpacing(6);
#endif
        vboxLayout->setContentsMargins(8, 8, 8, 8);
        vboxLayout->setObjectName(QString::fromUtf8("vboxLayout"));
        groupBox = new QGroupBox(Form);
        groupBox->setObjectName(QString::fromUtf8("groupBox"));
        gridLayout = new QGridLayout(groupBox);
#ifndef Q_OS_MAC
        gridLayout->setSpacing(6);
#endif
        gridLayout->setContentsMargins(8, 8, 8, 8);
        gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
        radioButton_2 = new QRadioButton(groupBox);
        radioButton_2->setObjectName(QString::fromUtf8("radioButton_2"));

        gridLayout->addWidget(radioButton_2, 1, 0, 1, 1);

        radioButton = new QRadioButton(groupBox);
        radioButton->setObjectName(QString::fromUtf8("radioButton"));
        radioButton->setChecked(true);

        gridLayout->addWidget(radioButton, 0, 0, 1, 1);

        checkBox_2 = new QCheckBox(groupBox);
        checkBox_2->setObjectName(QString::fromUtf8("checkBox_2"));
        checkBox_2->setChecked(true);

        gridLayout->addWidget(checkBox_2, 1, 1, 1, 1);

        spacerItem = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

        gridLayout->addItem(spacerItem, 5, 0, 1, 1);

        checkBox = new QCheckBox(groupBox);
        checkBox->setObjectName(QString::fromUtf8("checkBox"));

        gridLayout->addWidget(checkBox, 0, 1, 1, 1);

        radioButton_2_2 = new QRadioButton(groupBox);
        radioButton_2_2->setObjectName(QString::fromUtf8("radioButton_2_2"));

        gridLayout->addWidget(radioButton_2_2, 2, 0, 1, 1);

        radioButton_3 = new QRadioButton(groupBox);
        radioButton_3->setObjectName(QString::fromUtf8("radioButton_3"));

        gridLayout->addWidget(radioButton_3, 3, 0, 1, 1);

        radioButton_4 = new QRadioButton(groupBox);
        radioButton_4->setObjectName(QString::fromUtf8("radioButton_4"));

        gridLayout->addWidget(radioButton_4, 4, 0, 1, 1);

        checkBox_3 = new QCheckBox(groupBox);
        checkBox_3->setObjectName(QString::fromUtf8("checkBox_3"));

        gridLayout->addWidget(checkBox_3, 2, 1, 1, 1);

        checkBox_4 = new QCheckBox(groupBox);
        checkBox_4->setObjectName(QString::fromUtf8("checkBox_4"));
        checkBox_4->setChecked(true);

        gridLayout->addWidget(checkBox_4, 3, 1, 1, 1);


        vboxLayout->addWidget(groupBox);


        retranslateUi(Form);

        QMetaObject::connectSlotsByName(Form);
    } // setupUi

    void retranslateUi(QWidget *Form)
    {
        Form->setWindowTitle(QCoreApplication::translate("Form", "Export Document", nullptr));
        groupBox->setTitle(QCoreApplication::translate("Form", "Export Options", nullptr));
        radioButton_2->setText(QCoreApplication::translate("Form", "&DocBook", nullptr));
        radioButton->setText(QCoreApplication::translate("Form", "&LaTeX", nullptr));
        checkBox_2->setText(QCoreApplication::translate("Form", "Include p&ictures", nullptr));
        checkBox->setText(QCoreApplication::translate("Form", "&Compress", nullptr));
        radioButton_2_2->setText(QCoreApplication::translate("Form", "&HTML", nullptr));
        radioButton_3->setText(QCoreApplication::translate("Form", "&PostScript", nullptr));
        radioButton_4->setText(QCoreApplication::translate("Form", "PD&F", nullptr));
        checkBox_3->setText(QCoreApplication::translate("Form", "Include &metadata", nullptr));
        checkBox_4->setText(QCoreApplication::translate("Form", "Create inde&x", nullptr));
    } // retranslateUi

};

namespace Ui {
    class Form: public Ui_Form {};
} // namespace Ui

QT_END_NAMESPACE

#endif // MYFORM_H

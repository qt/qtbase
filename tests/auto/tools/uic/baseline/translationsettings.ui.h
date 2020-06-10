/********************************************************************************
** Form generated from reading UI file 'translationsettings.ui'
**
** Created by: Qt User Interface Compiler version 6.0.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef TRANSLATIONSETTINGS_H
#define TRANSLATIONSETTINGS_H

#include <QtCore/QVariant>
#include <QtWidgets/QAbstractButton>
#include <QtWidgets/QApplication>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QDialog>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QLabel>
#include <QtWidgets/QVBoxLayout>

QT_BEGIN_NAMESPACE

class Ui_TranslationSettings
{
public:
    QVBoxLayout *vboxLayout;
    QGroupBox *groupBox;
    QGridLayout *gridLayout;
    QComboBox *cbLanguageList;
    QLabel *label;
    QComboBox *cbCountryList;
    QLabel *lblCountry;
    QDialogButtonBox *buttonBox;

    void setupUi(QDialog *TranslationSettings)
    {
        if (TranslationSettings->objectName().isEmpty())
            TranslationSettings->setObjectName("TranslationSettings");
        TranslationSettings->resize(346, 125);
        vboxLayout = new QVBoxLayout(TranslationSettings);
#ifndef Q_OS_MAC
        vboxLayout->setSpacing(6);
#endif
#ifndef Q_OS_MAC
        vboxLayout->setContentsMargins(9, 9, 9, 9);
#endif
        vboxLayout->setObjectName("vboxLayout");
        groupBox = new QGroupBox(TranslationSettings);
        groupBox->setObjectName("groupBox");
        gridLayout = new QGridLayout(groupBox);
#ifndef Q_OS_MAC
        gridLayout->setSpacing(6);
#endif
#ifndef Q_OS_MAC
        gridLayout->setContentsMargins(9, 9, 9, 9);
#endif
        gridLayout->setObjectName("gridLayout");
        cbLanguageList = new QComboBox(groupBox);
        cbLanguageList->setObjectName("cbLanguageList");

        gridLayout->addWidget(cbLanguageList, 0, 1, 1, 1);

        label = new QLabel(groupBox);
        label->setObjectName("label");

        gridLayout->addWidget(label, 0, 0, 1, 1);

        cbCountryList = new QComboBox(groupBox);
        cbCountryList->setObjectName("cbCountryList");

        gridLayout->addWidget(cbCountryList, 1, 1, 1, 1);

        lblCountry = new QLabel(groupBox);
        lblCountry->setObjectName("lblCountry");

        gridLayout->addWidget(lblCountry, 1, 0, 1, 1);


        vboxLayout->addWidget(groupBox);

        buttonBox = new QDialogButtonBox(TranslationSettings);
        buttonBox->setObjectName("buttonBox");
        buttonBox->setOrientation(Qt::Horizontal);
        buttonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::NoButton|QDialogButtonBox::Ok);

        vboxLayout->addWidget(buttonBox);

#if QT_CONFIG(shortcut)
        label->setBuddy(cbLanguageList);
#endif // QT_CONFIG(shortcut)

        retranslateUi(TranslationSettings);
        QObject::connect(buttonBox, &QDialogButtonBox::accepted, TranslationSettings, qOverload<>(&QDialog::accept));
        QObject::connect(buttonBox, &QDialogButtonBox::rejected, TranslationSettings, qOverload<>(&QDialog::reject));

        QMetaObject::connectSlotsByName(TranslationSettings);
    } // setupUi

    void retranslateUi(QDialog *TranslationSettings)
    {
        TranslationSettings->setWindowTitle(QCoreApplication::translate("TranslationSettings", "Qt Linguist - Translation file settings", nullptr));
        groupBox->setTitle(QCoreApplication::translate("TranslationSettings", "Target language", nullptr));
        label->setText(QCoreApplication::translate("TranslationSettings", "Language", nullptr));
        lblCountry->setText(QCoreApplication::translate("TranslationSettings", "Country/Region", nullptr));
    } // retranslateUi

};

namespace Ui {
    class TranslationSettings: public Ui_TranslationSettings {};
} // namespace Ui

QT_END_NAMESPACE

#endif // TRANSLATIONSETTINGS_H

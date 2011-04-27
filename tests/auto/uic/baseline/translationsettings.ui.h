/********************************************************************************
** Form generated from reading UI file 'translationsettings.ui'
**
** Created: Fri Sep 4 10:17:15 2009
**      by: Qt User Interface Compiler version 4.6.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef TRANSLATIONSETTINGS_H
#define TRANSLATIONSETTINGS_H

#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QComboBox>
#include <QtGui/QDialog>
#include <QtGui/QDialogButtonBox>
#include <QtGui/QGridLayout>
#include <QtGui/QGroupBox>
#include <QtGui/QHeaderView>
#include <QtGui/QLabel>
#include <QtGui/QVBoxLayout>

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
            TranslationSettings->setObjectName(QString::fromUtf8("TranslationSettings"));
        TranslationSettings->resize(346, 125);
        vboxLayout = new QVBoxLayout(TranslationSettings);
#ifndef Q_OS_MAC
        vboxLayout->setSpacing(6);
#endif
#ifndef Q_OS_MAC
        vboxLayout->setContentsMargins(9, 9, 9, 9);
#endif
        vboxLayout->setObjectName(QString::fromUtf8("vboxLayout"));
        groupBox = new QGroupBox(TranslationSettings);
        groupBox->setObjectName(QString::fromUtf8("groupBox"));
        gridLayout = new QGridLayout(groupBox);
#ifndef Q_OS_MAC
        gridLayout->setSpacing(6);
#endif
#ifndef Q_OS_MAC
        gridLayout->setContentsMargins(9, 9, 9, 9);
#endif
        gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
        cbLanguageList = new QComboBox(groupBox);
        cbLanguageList->setObjectName(QString::fromUtf8("cbLanguageList"));

        gridLayout->addWidget(cbLanguageList, 0, 1, 1, 1);

        label = new QLabel(groupBox);
        label->setObjectName(QString::fromUtf8("label"));

        gridLayout->addWidget(label, 0, 0, 1, 1);

        cbCountryList = new QComboBox(groupBox);
        cbCountryList->setObjectName(QString::fromUtf8("cbCountryList"));

        gridLayout->addWidget(cbCountryList, 1, 1, 1, 1);

        lblCountry = new QLabel(groupBox);
        lblCountry->setObjectName(QString::fromUtf8("lblCountry"));

        gridLayout->addWidget(lblCountry, 1, 0, 1, 1);


        vboxLayout->addWidget(groupBox);

        buttonBox = new QDialogButtonBox(TranslationSettings);
        buttonBox->setObjectName(QString::fromUtf8("buttonBox"));
        buttonBox->setOrientation(Qt::Horizontal);
        buttonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::NoButton|QDialogButtonBox::Ok);

        vboxLayout->addWidget(buttonBox);

#ifndef QT_NO_SHORTCUT
        label->setBuddy(cbLanguageList);
#endif // QT_NO_SHORTCUT

        retranslateUi(TranslationSettings);
        QObject::connect(buttonBox, SIGNAL(accepted()), TranslationSettings, SLOT(accept()));
        QObject::connect(buttonBox, SIGNAL(rejected()), TranslationSettings, SLOT(reject()));

        QMetaObject::connectSlotsByName(TranslationSettings);
    } // setupUi

    void retranslateUi(QDialog *TranslationSettings)
    {
        TranslationSettings->setWindowTitle(QApplication::translate("TranslationSettings", "Qt Linguist - Translation file settings", 0, QApplication::UnicodeUTF8));
        groupBox->setTitle(QApplication::translate("TranslationSettings", "Target language", 0, QApplication::UnicodeUTF8));
        label->setText(QApplication::translate("TranslationSettings", "Language", 0, QApplication::UnicodeUTF8));
        lblCountry->setText(QApplication::translate("TranslationSettings", "Country/Region", 0, QApplication::UnicodeUTF8));
    } // retranslateUi

};

namespace Ui {
    class TranslationSettings: public Ui_TranslationSettings {};
} // namespace Ui

QT_END_NAMESPACE

#endif // TRANSLATIONSETTINGS_H

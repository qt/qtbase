/********************************************************************************
** Form generated from reading UI file 'wateringconfigdialog.ui'
**
** Created by: Qt User Interface Compiler version 5.12.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef WATERINGCONFIGDIALOG_H
#define WATERINGCONFIGDIALOG_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QDialog>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QFrame>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QTimeEdit>
#include <QtWidgets/QVBoxLayout>
#include "helpbrowser.h"

QT_BEGIN_NAMESPACE

class Ui_WateringConfigDialog
{
public:
    QVBoxLayout *vboxLayout;
    QGridLayout *gridLayout;
    QLabel *label_3;
    QComboBox *plantComboBox;
    QSpacerItem *spacerItem;
    QLabel *label_2;
    QCheckBox *temperatureCheckBox;
    QSpacerItem *spacerItem1;
    QSpinBox *temperatureSpinBox;
    QSpacerItem *spacerItem2;
    QCheckBox *rainCheckBox;
    QSpacerItem *spacerItem3;
    QSpinBox *rainSpinBox;
    QSpacerItem *spacerItem4;
    QSpacerItem *spacerItem5;
    QLabel *label;
    QTimeEdit *startTimeEdit;
    QLabel *label_4;
    QSpinBox *amountSpinBox;
    QLabel *label_5;
    QComboBox *sourceComboBox;
    QLabel *label_6;
    QCheckBox *filterCheckBox;
    QSpacerItem *spacerItem6;
    QSpacerItem *spacerItem7;
    QGridLayout *gridLayout1;
    QSpacerItem *spacerItem8;
    HelpBrowser *helpBrowser;
    QLabel *helpLabel;
    QFrame *line;
    QDialogButtonBox *buttonBox;

    void setupUi(QDialog *WateringConfigDialog)
    {
        if (WateringConfigDialog->objectName().isEmpty())
            WateringConfigDialog->setObjectName(QString::fromUtf8("WateringConfigDialog"));
        WateringConfigDialog->resize(334, 550);
        vboxLayout = new QVBoxLayout(WateringConfigDialog);
        vboxLayout->setObjectName(QString::fromUtf8("vboxLayout"));
        gridLayout = new QGridLayout();
        gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
        label_3 = new QLabel(WateringConfigDialog);
        label_3->setObjectName(QString::fromUtf8("label_3"));

        gridLayout->addWidget(label_3, 0, 0, 1, 1);

        plantComboBox = new QComboBox(WateringConfigDialog);
        plantComboBox->addItem(QString());
        plantComboBox->addItem(QString());
        plantComboBox->addItem(QString());
        plantComboBox->addItem(QString());
        plantComboBox->addItem(QString());
        plantComboBox->addItem(QString());
        plantComboBox->setObjectName(QString::fromUtf8("plantComboBox"));
        QSizePolicy sizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(plantComboBox->sizePolicy().hasHeightForWidth());
        plantComboBox->setSizePolicy(sizePolicy);

        gridLayout->addWidget(plantComboBox, 0, 1, 1, 3);

        spacerItem = new QSpacerItem(67, 16, QSizePolicy::Minimum, QSizePolicy::Fixed);

        gridLayout->addItem(spacerItem, 1, 0, 1, 1);

        label_2 = new QLabel(WateringConfigDialog);
        label_2->setObjectName(QString::fromUtf8("label_2"));

        gridLayout->addWidget(label_2, 2, 0, 1, 1);

        temperatureCheckBox = new QCheckBox(WateringConfigDialog);
        temperatureCheckBox->setObjectName(QString::fromUtf8("temperatureCheckBox"));

        gridLayout->addWidget(temperatureCheckBox, 3, 1, 1, 3);

        spacerItem1 = new QSpacerItem(16, 20, QSizePolicy::Fixed, QSizePolicy::Minimum);

        gridLayout->addItem(spacerItem1, 4, 1, 1, 1);

        temperatureSpinBox = new QSpinBox(WateringConfigDialog);
        temperatureSpinBox->setObjectName(QString::fromUtf8("temperatureSpinBox"));
        temperatureSpinBox->setEnabled(false);
        temperatureSpinBox->setMinimum(10);
        temperatureSpinBox->setMaximum(60);
        temperatureSpinBox->setValue(20);

        gridLayout->addWidget(temperatureSpinBox, 4, 2, 1, 1);

        spacerItem2 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        gridLayout->addItem(spacerItem2, 4, 3, 1, 1);

        rainCheckBox = new QCheckBox(WateringConfigDialog);
        rainCheckBox->setObjectName(QString::fromUtf8("rainCheckBox"));

        gridLayout->addWidget(rainCheckBox, 5, 1, 1, 3);

        spacerItem3 = new QSpacerItem(16, 20, QSizePolicy::Fixed, QSizePolicy::Minimum);

        gridLayout->addItem(spacerItem3, 6, 1, 1, 1);

        rainSpinBox = new QSpinBox(WateringConfigDialog);
        rainSpinBox->setObjectName(QString::fromUtf8("rainSpinBox"));
        rainSpinBox->setEnabled(false);
        rainSpinBox->setMinimum(1);

        gridLayout->addWidget(rainSpinBox, 6, 2, 1, 1);

        spacerItem4 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        gridLayout->addItem(spacerItem4, 6, 3, 1, 1);

        spacerItem5 = new QSpacerItem(20, 20, QSizePolicy::Minimum, QSizePolicy::Fixed);

        gridLayout->addItem(spacerItem5, 7, 2, 1, 1);

        label = new QLabel(WateringConfigDialog);
        label->setObjectName(QString::fromUtf8("label"));

        gridLayout->addWidget(label, 8, 0, 1, 1);

        startTimeEdit = new QTimeEdit(WateringConfigDialog);
        startTimeEdit->setObjectName(QString::fromUtf8("startTimeEdit"));

        gridLayout->addWidget(startTimeEdit, 8, 1, 1, 3);

        label_4 = new QLabel(WateringConfigDialog);
        label_4->setObjectName(QString::fromUtf8("label_4"));

        gridLayout->addWidget(label_4, 9, 0, 1, 1);

        amountSpinBox = new QSpinBox(WateringConfigDialog);
        amountSpinBox->setObjectName(QString::fromUtf8("amountSpinBox"));
        amountSpinBox->setMinimum(100);
        amountSpinBox->setMaximum(10000);
        amountSpinBox->setSingleStep(100);
        amountSpinBox->setValue(1000);

        gridLayout->addWidget(amountSpinBox, 9, 1, 1, 3);

        label_5 = new QLabel(WateringConfigDialog);
        label_5->setObjectName(QString::fromUtf8("label_5"));

        gridLayout->addWidget(label_5, 10, 0, 1, 1);

        sourceComboBox = new QComboBox(WateringConfigDialog);
        sourceComboBox->addItem(QString());
        sourceComboBox->addItem(QString());
        sourceComboBox->addItem(QString());
        sourceComboBox->addItem(QString());
        sourceComboBox->setObjectName(QString::fromUtf8("sourceComboBox"));

        gridLayout->addWidget(sourceComboBox, 10, 1, 1, 3);

        label_6 = new QLabel(WateringConfigDialog);
        label_6->setObjectName(QString::fromUtf8("label_6"));

        gridLayout->addWidget(label_6, 11, 0, 1, 1);

        filterCheckBox = new QCheckBox(WateringConfigDialog);
        filterCheckBox->setObjectName(QString::fromUtf8("filterCheckBox"));

        gridLayout->addWidget(filterCheckBox, 11, 1, 1, 2);

        spacerItem6 = new QSpacerItem(20, 10, QSizePolicy::Minimum, QSizePolicy::Fixed);

        gridLayout->addItem(spacerItem6, 12, 0, 1, 1);

        spacerItem7 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        gridLayout->addItem(spacerItem7, 4, 4, 1, 1);


        vboxLayout->addLayout(gridLayout);

        gridLayout1 = new QGridLayout();
        gridLayout1->setObjectName(QString::fromUtf8("gridLayout1"));
        spacerItem8 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        gridLayout1->addItem(spacerItem8, 0, 1, 1, 1);

        helpBrowser = new HelpBrowser(WateringConfigDialog);
        helpBrowser->setObjectName(QString::fromUtf8("helpBrowser"));

        gridLayout1->addWidget(helpBrowser, 1, 0, 1, 2);

        helpLabel = new QLabel(WateringConfigDialog);
        helpLabel->setObjectName(QString::fromUtf8("helpLabel"));

        gridLayout1->addWidget(helpLabel, 0, 0, 1, 1);


        vboxLayout->addLayout(gridLayout1);

        line = new QFrame(WateringConfigDialog);
        line->setObjectName(QString::fromUtf8("line"));
        line->setFrameShape(QFrame::HLine);
        line->setFrameShadow(QFrame::Sunken);

        vboxLayout->addWidget(line);

        buttonBox = new QDialogButtonBox(WateringConfigDialog);
        buttonBox->setObjectName(QString::fromUtf8("buttonBox"));
        buttonBox->setOrientation(Qt::Horizontal);
        buttonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::NoButton|QDialogButtonBox::Ok);

        vboxLayout->addWidget(buttonBox);


        retranslateUi(WateringConfigDialog);
        QObject::connect(buttonBox, SIGNAL(accepted()), WateringConfigDialog, SLOT(accept()));
        QObject::connect(buttonBox, SIGNAL(rejected()), WateringConfigDialog, SLOT(reject()));
        QObject::connect(temperatureCheckBox, SIGNAL(toggled(bool)), temperatureSpinBox, SLOT(setEnabled(bool)));
        QObject::connect(rainCheckBox, SIGNAL(toggled(bool)), rainSpinBox, SLOT(setEnabled(bool)));

        QMetaObject::connectSlotsByName(WateringConfigDialog);
    } // setupUi

    void retranslateUi(QDialog *WateringConfigDialog)
    {
        WateringConfigDialog->setWindowTitle(QCoreApplication::translate("WateringConfigDialog", "Watering Configuration", nullptr));
        label_3->setText(QCoreApplication::translate("WateringConfigDialog", "Plant:", nullptr));
        plantComboBox->setItemText(0, QCoreApplication::translate("WateringConfigDialog", "Squash", nullptr));
        plantComboBox->setItemText(1, QCoreApplication::translate("WateringConfigDialog", "Bean", nullptr));
        plantComboBox->setItemText(2, QCoreApplication::translate("WateringConfigDialog", "Carrot", nullptr));
        plantComboBox->setItemText(3, QCoreApplication::translate("WateringConfigDialog", "Strawberry", nullptr));
        plantComboBox->setItemText(4, QCoreApplication::translate("WateringConfigDialog", "Raspberry", nullptr));
        plantComboBox->setItemText(5, QCoreApplication::translate("WateringConfigDialog", "Blueberry", nullptr));

        label_2->setText(QCoreApplication::translate("WateringConfigDialog", "Water when:", nullptr));
        temperatureCheckBox->setText(QCoreApplication::translate("WateringConfigDialog", "Temperature is higher than:", nullptr));
        temperatureSpinBox->setSpecialValueText(QString());
        temperatureSpinBox->setSuffix(QCoreApplication::translate("WateringConfigDialog", "C", nullptr));
        rainCheckBox->setText(QCoreApplication::translate("WateringConfigDialog", "Rain less than:", nullptr));
        rainSpinBox->setSpecialValueText(QString());
        rainSpinBox->setSuffix(QCoreApplication::translate("WateringConfigDialog", "mm", nullptr));
        label->setText(QCoreApplication::translate("WateringConfigDialog", "Starting Time:", nullptr));
        label_4->setText(QCoreApplication::translate("WateringConfigDialog", "Amount:", nullptr));
        amountSpinBox->setSuffix(QCoreApplication::translate("WateringConfigDialog", "l", nullptr));
        label_5->setText(QCoreApplication::translate("WateringConfigDialog", "Source:", nullptr));
        sourceComboBox->setItemText(0, QCoreApplication::translate("WateringConfigDialog", "Foundain", nullptr));
        sourceComboBox->setItemText(1, QCoreApplication::translate("WateringConfigDialog", "River", nullptr));
        sourceComboBox->setItemText(2, QCoreApplication::translate("WateringConfigDialog", "Lake", nullptr));
        sourceComboBox->setItemText(3, QCoreApplication::translate("WateringConfigDialog", "Public Water System", nullptr));

        label_6->setText(QCoreApplication::translate("WateringConfigDialog", "Filter:", nullptr));
        filterCheckBox->setText(QString());
        helpLabel->setText(QCoreApplication::translate("WateringConfigDialog", "<a href=\"test\">Show Details</a>", nullptr));
    } // retranslateUi

};

namespace Ui {
    class WateringConfigDialog: public Ui_WateringConfigDialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // WATERINGCONFIGDIALOG_H

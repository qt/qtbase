/********************************************************************************
** Form generated from reading UI file 'wateringconfigdialog.ui'
**
** Created by: Qt User Interface Compiler version 5.0.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef WATERINGCONFIGDIALOG_H
#define WATERINGCONFIGDIALOG_H

#include <QtCore/QVariant>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QDialog>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QFrame>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QHeaderView>
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
            WateringConfigDialog->setObjectName(QStringLiteral("WateringConfigDialog"));
        WateringConfigDialog->resize(334, 550);
        vboxLayout = new QVBoxLayout(WateringConfigDialog);
        vboxLayout->setObjectName(QStringLiteral("vboxLayout"));
        gridLayout = new QGridLayout();
        gridLayout->setObjectName(QStringLiteral("gridLayout"));
        label_3 = new QLabel(WateringConfigDialog);
        label_3->setObjectName(QStringLiteral("label_3"));

        gridLayout->addWidget(label_3, 0, 0, 1, 1);

        plantComboBox = new QComboBox(WateringConfigDialog);
        plantComboBox->setObjectName(QStringLiteral("plantComboBox"));
        QSizePolicy sizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(plantComboBox->sizePolicy().hasHeightForWidth());
        plantComboBox->setSizePolicy(sizePolicy);

        gridLayout->addWidget(plantComboBox, 0, 1, 1, 3);

        spacerItem = new QSpacerItem(67, 16, QSizePolicy::Minimum, QSizePolicy::Fixed);

        gridLayout->addItem(spacerItem, 1, 0, 1, 1);

        label_2 = new QLabel(WateringConfigDialog);
        label_2->setObjectName(QStringLiteral("label_2"));

        gridLayout->addWidget(label_2, 2, 0, 1, 1);

        temperatureCheckBox = new QCheckBox(WateringConfigDialog);
        temperatureCheckBox->setObjectName(QStringLiteral("temperatureCheckBox"));

        gridLayout->addWidget(temperatureCheckBox, 3, 1, 1, 3);

        spacerItem1 = new QSpacerItem(16, 20, QSizePolicy::Fixed, QSizePolicy::Minimum);

        gridLayout->addItem(spacerItem1, 4, 1, 1, 1);

        temperatureSpinBox = new QSpinBox(WateringConfigDialog);
        temperatureSpinBox->setObjectName(QStringLiteral("temperatureSpinBox"));
        temperatureSpinBox->setEnabled(false);
        temperatureSpinBox->setMinimum(10);
        temperatureSpinBox->setMaximum(60);
        temperatureSpinBox->setValue(20);

        gridLayout->addWidget(temperatureSpinBox, 4, 2, 1, 1);

        spacerItem2 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        gridLayout->addItem(spacerItem2, 4, 3, 1, 1);

        rainCheckBox = new QCheckBox(WateringConfigDialog);
        rainCheckBox->setObjectName(QStringLiteral("rainCheckBox"));

        gridLayout->addWidget(rainCheckBox, 5, 1, 1, 3);

        spacerItem3 = new QSpacerItem(16, 20, QSizePolicy::Fixed, QSizePolicy::Minimum);

        gridLayout->addItem(spacerItem3, 6, 1, 1, 1);

        rainSpinBox = new QSpinBox(WateringConfigDialog);
        rainSpinBox->setObjectName(QStringLiteral("rainSpinBox"));
        rainSpinBox->setEnabled(false);
        rainSpinBox->setMinimum(1);

        gridLayout->addWidget(rainSpinBox, 6, 2, 1, 1);

        spacerItem4 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        gridLayout->addItem(spacerItem4, 6, 3, 1, 1);

        spacerItem5 = new QSpacerItem(20, 20, QSizePolicy::Minimum, QSizePolicy::Fixed);

        gridLayout->addItem(spacerItem5, 7, 2, 1, 1);

        label = new QLabel(WateringConfigDialog);
        label->setObjectName(QStringLiteral("label"));

        gridLayout->addWidget(label, 8, 0, 1, 1);

        startTimeEdit = new QTimeEdit(WateringConfigDialog);
        startTimeEdit->setObjectName(QStringLiteral("startTimeEdit"));

        gridLayout->addWidget(startTimeEdit, 8, 1, 1, 3);

        label_4 = new QLabel(WateringConfigDialog);
        label_4->setObjectName(QStringLiteral("label_4"));

        gridLayout->addWidget(label_4, 9, 0, 1, 1);

        amountSpinBox = new QSpinBox(WateringConfigDialog);
        amountSpinBox->setObjectName(QStringLiteral("amountSpinBox"));
        amountSpinBox->setMinimum(100);
        amountSpinBox->setMaximum(10000);
        amountSpinBox->setSingleStep(100);
        amountSpinBox->setValue(1000);

        gridLayout->addWidget(amountSpinBox, 9, 1, 1, 3);

        label_5 = new QLabel(WateringConfigDialog);
        label_5->setObjectName(QStringLiteral("label_5"));

        gridLayout->addWidget(label_5, 10, 0, 1, 1);

        sourceComboBox = new QComboBox(WateringConfigDialog);
        sourceComboBox->setObjectName(QStringLiteral("sourceComboBox"));

        gridLayout->addWidget(sourceComboBox, 10, 1, 1, 3);

        label_6 = new QLabel(WateringConfigDialog);
        label_6->setObjectName(QStringLiteral("label_6"));

        gridLayout->addWidget(label_6, 11, 0, 1, 1);

        filterCheckBox = new QCheckBox(WateringConfigDialog);
        filterCheckBox->setObjectName(QStringLiteral("filterCheckBox"));

        gridLayout->addWidget(filterCheckBox, 11, 1, 1, 2);

        spacerItem6 = new QSpacerItem(20, 10, QSizePolicy::Minimum, QSizePolicy::Fixed);

        gridLayout->addItem(spacerItem6, 12, 0, 1, 1);

        spacerItem7 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        gridLayout->addItem(spacerItem7, 4, 4, 1, 1);


        vboxLayout->addLayout(gridLayout);

        gridLayout1 = new QGridLayout();
        gridLayout1->setObjectName(QStringLiteral("gridLayout1"));
        spacerItem8 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        gridLayout1->addItem(spacerItem8, 0, 1, 1, 1);

        helpBrowser = new HelpBrowser(WateringConfigDialog);
        helpBrowser->setObjectName(QStringLiteral("helpBrowser"));

        gridLayout1->addWidget(helpBrowser, 1, 0, 1, 2);

        helpLabel = new QLabel(WateringConfigDialog);
        helpLabel->setObjectName(QStringLiteral("helpLabel"));

        gridLayout1->addWidget(helpLabel, 0, 0, 1, 1);


        vboxLayout->addLayout(gridLayout1);

        line = new QFrame(WateringConfigDialog);
        line->setObjectName(QStringLiteral("line"));
        line->setFrameShape(QFrame::HLine);
        line->setFrameShadow(QFrame::Sunken);

        vboxLayout->addWidget(line);

        buttonBox = new QDialogButtonBox(WateringConfigDialog);
        buttonBox->setObjectName(QStringLiteral("buttonBox"));
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
        WateringConfigDialog->setWindowTitle(QApplication::translate("WateringConfigDialog", "Watering Configuration", Q_NULLPTR));
        label_3->setText(QApplication::translate("WateringConfigDialog", "Plant:", Q_NULLPTR));
        plantComboBox->clear();
        plantComboBox->insertItems(0, QStringList()
         << QApplication::translate("WateringConfigDialog", "Squash", Q_NULLPTR)
         << QApplication::translate("WateringConfigDialog", "Bean", Q_NULLPTR)
         << QApplication::translate("WateringConfigDialog", "Carrot", Q_NULLPTR)
         << QApplication::translate("WateringConfigDialog", "Strawberry", Q_NULLPTR)
         << QApplication::translate("WateringConfigDialog", "Raspberry", Q_NULLPTR)
         << QApplication::translate("WateringConfigDialog", "Blueberry", Q_NULLPTR)
        );
        label_2->setText(QApplication::translate("WateringConfigDialog", "Water when:", Q_NULLPTR));
        temperatureCheckBox->setText(QApplication::translate("WateringConfigDialog", "Temperature is higher than:", Q_NULLPTR));
        temperatureSpinBox->setSpecialValueText(QString());
        temperatureSpinBox->setSuffix(QApplication::translate("WateringConfigDialog", "C", Q_NULLPTR));
        rainCheckBox->setText(QApplication::translate("WateringConfigDialog", "Rain less than:", Q_NULLPTR));
        rainSpinBox->setSpecialValueText(QString());
        rainSpinBox->setSuffix(QApplication::translate("WateringConfigDialog", "mm", Q_NULLPTR));
        label->setText(QApplication::translate("WateringConfigDialog", "Starting Time:", Q_NULLPTR));
        label_4->setText(QApplication::translate("WateringConfigDialog", "Amount:", Q_NULLPTR));
        amountSpinBox->setSuffix(QApplication::translate("WateringConfigDialog", "l", Q_NULLPTR));
        label_5->setText(QApplication::translate("WateringConfigDialog", "Source:", Q_NULLPTR));
        sourceComboBox->clear();
        sourceComboBox->insertItems(0, QStringList()
         << QApplication::translate("WateringConfigDialog", "Foundain", Q_NULLPTR)
         << QApplication::translate("WateringConfigDialog", "River", Q_NULLPTR)
         << QApplication::translate("WateringConfigDialog", "Lake", Q_NULLPTR)
         << QApplication::translate("WateringConfigDialog", "Public Water System", Q_NULLPTR)
        );
        label_6->setText(QApplication::translate("WateringConfigDialog", "Filter:", Q_NULLPTR));
        filterCheckBox->setText(QString());
        helpLabel->setText(QApplication::translate("WateringConfigDialog", "<a href=\"test\">Show Details</a>", Q_NULLPTR));
    } // retranslateUi

};

namespace Ui {
    class WateringConfigDialog: public Ui_WateringConfigDialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // WATERINGCONFIGDIALOG_H

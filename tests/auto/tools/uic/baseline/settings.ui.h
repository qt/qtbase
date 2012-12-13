/********************************************************************************
** Form generated from reading UI file 'settings.ui'
**
** Created by: Qt User Interface Compiler version 5.0.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef SETTINGS_H
#define SETTINGS_H

#include <QtCore/QVariant>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QDialog>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QSlider>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QVBoxLayout>

QT_BEGIN_NAMESPACE

class Ui_Dialog
{
public:
    QVBoxLayout *verticalLayout;
    QHBoxLayout *hboxLayout;
    QLabel *label;
    QComboBox *deviceCombo;
    QHBoxLayout *hboxLayout1;
    QLabel *label_6;
    QComboBox *audioEffectsCombo;
    QHBoxLayout *hboxLayout2;
    QLabel *crossFadeLabel;
    QVBoxLayout *vboxLayout;
    QSlider *crossFadeSlider;
    QHBoxLayout *hboxLayout3;
    QLabel *label_3;
    QSpacerItem *spacerItem;
    QLabel *label_5;
    QSpacerItem *spacerItem1;
    QLabel *label_4;
    QDialogButtonBox *buttonBox;

    void setupUi(QDialog *Dialog)
    {
        if (Dialog->objectName().isEmpty())
            Dialog->setObjectName(QStringLiteral("Dialog"));
        Dialog->resize(392, 176);
        verticalLayout = new QVBoxLayout(Dialog);
        verticalLayout->setObjectName(QStringLiteral("verticalLayout"));
        hboxLayout = new QHBoxLayout();
        hboxLayout->setObjectName(QStringLiteral("hboxLayout"));
        label = new QLabel(Dialog);
        label->setObjectName(QStringLiteral("label"));
        QSizePolicy sizePolicy(QSizePolicy::Maximum, QSizePolicy::Preferred);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(label->sizePolicy().hasHeightForWidth());
        label->setSizePolicy(sizePolicy);
        label->setMinimumSize(QSize(90, 0));
        label->setAlignment(Qt::AlignLeading|Qt::AlignLeft|Qt::AlignTop);

        hboxLayout->addWidget(label);

        deviceCombo = new QComboBox(Dialog);
        deviceCombo->setObjectName(QStringLiteral("deviceCombo"));
        QSizePolicy sizePolicy1(QSizePolicy::Minimum, QSizePolicy::Fixed);
        sizePolicy1.setHorizontalStretch(0);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(deviceCombo->sizePolicy().hasHeightForWidth());
        deviceCombo->setSizePolicy(sizePolicy1);

        hboxLayout->addWidget(deviceCombo);


        verticalLayout->addLayout(hboxLayout);

        hboxLayout1 = new QHBoxLayout();
        hboxLayout1->setObjectName(QStringLiteral("hboxLayout1"));
        label_6 = new QLabel(Dialog);
        label_6->setObjectName(QStringLiteral("label_6"));
        sizePolicy.setHeightForWidth(label_6->sizePolicy().hasHeightForWidth());
        label_6->setSizePolicy(sizePolicy);
        label_6->setMinimumSize(QSize(90, 0));
        label_6->setAlignment(Qt::AlignLeading|Qt::AlignLeft|Qt::AlignTop);

        hboxLayout1->addWidget(label_6);

        audioEffectsCombo = new QComboBox(Dialog);
        audioEffectsCombo->setObjectName(QStringLiteral("audioEffectsCombo"));
        sizePolicy1.setHeightForWidth(audioEffectsCombo->sizePolicy().hasHeightForWidth());
        audioEffectsCombo->setSizePolicy(sizePolicy1);

        hboxLayout1->addWidget(audioEffectsCombo);


        verticalLayout->addLayout(hboxLayout1);

        hboxLayout2 = new QHBoxLayout();
        hboxLayout2->setObjectName(QStringLiteral("hboxLayout2"));
        crossFadeLabel = new QLabel(Dialog);
        crossFadeLabel->setObjectName(QStringLiteral("crossFadeLabel"));
        sizePolicy.setHeightForWidth(crossFadeLabel->sizePolicy().hasHeightForWidth());
        crossFadeLabel->setSizePolicy(sizePolicy);
        crossFadeLabel->setMinimumSize(QSize(90, 0));
        crossFadeLabel->setAlignment(Qt::AlignLeading|Qt::AlignLeft|Qt::AlignTop);

        hboxLayout2->addWidget(crossFadeLabel);

        vboxLayout = new QVBoxLayout();
        vboxLayout->setObjectName(QStringLiteral("vboxLayout"));
        crossFadeSlider = new QSlider(Dialog);
        crossFadeSlider->setObjectName(QStringLiteral("crossFadeSlider"));
        sizePolicy1.setHeightForWidth(crossFadeSlider->sizePolicy().hasHeightForWidth());
        crossFadeSlider->setSizePolicy(sizePolicy1);
        crossFadeSlider->setMinimum(-20);
        crossFadeSlider->setMaximum(20);
        crossFadeSlider->setSingleStep(1);
        crossFadeSlider->setPageStep(2);
        crossFadeSlider->setValue(0);
        crossFadeSlider->setOrientation(Qt::Horizontal);
        crossFadeSlider->setTickPosition(QSlider::TicksBelow);

        vboxLayout->addWidget(crossFadeSlider);

        hboxLayout3 = new QHBoxLayout();
        hboxLayout3->setObjectName(QStringLiteral("hboxLayout3"));
        label_3 = new QLabel(Dialog);
        label_3->setObjectName(QStringLiteral("label_3"));
        QFont font;
        font.setPointSize(9);
        label_3->setFont(font);

        hboxLayout3->addWidget(label_3);

        spacerItem = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        hboxLayout3->addItem(spacerItem);

        label_5 = new QLabel(Dialog);
        label_5->setObjectName(QStringLiteral("label_5"));
        label_5->setFont(font);

        hboxLayout3->addWidget(label_5);

        spacerItem1 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        hboxLayout3->addItem(spacerItem1);

        label_4 = new QLabel(Dialog);
        label_4->setObjectName(QStringLiteral("label_4"));
        label_4->setFont(font);

        hboxLayout3->addWidget(label_4);


        vboxLayout->addLayout(hboxLayout3);


        hboxLayout2->addLayout(vboxLayout);


        verticalLayout->addLayout(hboxLayout2);

        buttonBox = new QDialogButtonBox(Dialog);
        buttonBox->setObjectName(QStringLiteral("buttonBox"));
        buttonBox->setOrientation(Qt::Horizontal);
        buttonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);

        verticalLayout->addWidget(buttonBox);


        retranslateUi(Dialog);
        QObject::connect(buttonBox, SIGNAL(accepted()), Dialog, SLOT(accept()));
        QObject::connect(buttonBox, SIGNAL(rejected()), Dialog, SLOT(reject()));

        QMetaObject::connectSlotsByName(Dialog);
    } // setupUi

    void retranslateUi(QDialog *Dialog)
    {
        Dialog->setWindowTitle(QApplication::translate("Dialog", "Dialog", 0));
        label->setText(QApplication::translate("Dialog", "Audio device:", 0));
        label_6->setText(QApplication::translate("Dialog", "Audio effect:", 0));
        crossFadeLabel->setText(QApplication::translate("Dialog", "Cross fade:", 0));
        label_3->setText(QApplication::translate("Dialog", "-10 Sec", 0));
        label_5->setText(QApplication::translate("Dialog", "0", 0));
        label_4->setText(QApplication::translate("Dialog", "10 Sec", 0));
    } // retranslateUi

};

namespace Ui {
    class Dialog: public Ui_Dialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // SETTINGS_H

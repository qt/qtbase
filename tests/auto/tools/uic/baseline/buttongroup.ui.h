/********************************************************************************
** Form generated from reading UI file 'buttongroup.ui'
**
** Created by: Qt User Interface Compiler version 6.0.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef BUTTONGROUP_H
#define BUTTONGROUP_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QDoubleSpinBox>
#include <QtWidgets/QFormLayout>
#include <QtWidgets/QGraphicsView>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QLabel>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QRadioButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_Form
{
public:
    QGridLayout *gridLayout;
    QListWidget *easingCurvePicker;
    QVBoxLayout *verticalLayout;
    QGroupBox *groupBox_2;
    QGridLayout *gridLayout_2;
    QRadioButton *lineRadio;
    QRadioButton *circleRadio;
    QGroupBox *groupBox;
    QFormLayout *formLayout;
    QLabel *label;
    QDoubleSpinBox *periodSpinBox;
    QDoubleSpinBox *amplitudeSpinBox;
    QLabel *label_3;
    QDoubleSpinBox *overshootSpinBox;
    QLabel *label_2;
    QSpacerItem *verticalSpacer;
    QGraphicsView *graphicsView;
    QButtonGroup *buttonGroup;

    void setupUi(QWidget *Form)
    {
        if (Form->objectName().isEmpty())
            Form->setObjectName("Form");
        Form->resize(545, 471);
        gridLayout = new QGridLayout(Form);
        gridLayout->setObjectName("gridLayout");
        easingCurvePicker = new QListWidget(Form);
        easingCurvePicker->setObjectName("easingCurvePicker");
        QSizePolicy sizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Preferred);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(easingCurvePicker->sizePolicy().hasHeightForWidth());
        easingCurvePicker->setSizePolicy(sizePolicy);
        easingCurvePicker->setMaximumSize(QSize(16777215, 120));
        easingCurvePicker->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        easingCurvePicker->setMovement(QListView::Static);
        easingCurvePicker->setProperty("isWrapping", QVariant(false));
        easingCurvePicker->setViewMode(QListView::IconMode);
        easingCurvePicker->setSelectionRectVisible(false);

        gridLayout->addWidget(easingCurvePicker, 0, 0, 1, 2);

        verticalLayout = new QVBoxLayout();
        verticalLayout->setObjectName("verticalLayout");
        groupBox_2 = new QGroupBox(Form);
        groupBox_2->setObjectName("groupBox_2");
        groupBox_2->setMaximumSize(QSize(16777215, 16777215));
        gridLayout_2 = new QGridLayout(groupBox_2);
        gridLayout_2->setObjectName("gridLayout_2");
        lineRadio = new QRadioButton(groupBox_2);
        buttonGroup = new QButtonGroup(Form);
        buttonGroup->setObjectName("buttonGroup");
        buttonGroup->addButton(lineRadio);
        lineRadio->setObjectName("lineRadio");
        lineRadio->setMaximumSize(QSize(16777215, 40));
        lineRadio->setLayoutDirection(Qt::LeftToRight);
        lineRadio->setChecked(true);

        gridLayout_2->addWidget(lineRadio, 0, 0, 1, 1);

        circleRadio = new QRadioButton(groupBox_2);
        buttonGroup->addButton(circleRadio);
        circleRadio->setObjectName("circleRadio");
        circleRadio->setMaximumSize(QSize(16777215, 40));

        gridLayout_2->addWidget(circleRadio, 1, 0, 1, 1);


        verticalLayout->addWidget(groupBox_2);

        groupBox = new QGroupBox(Form);
        groupBox->setObjectName("groupBox");
        QSizePolicy sizePolicy1(QSizePolicy::Policy::Fixed, QSizePolicy::Policy::Preferred);
        sizePolicy1.setHorizontalStretch(0);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(groupBox->sizePolicy().hasHeightForWidth());
        groupBox->setSizePolicy(sizePolicy1);
        formLayout = new QFormLayout(groupBox);
        formLayout->setObjectName("formLayout");
        formLayout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
        label = new QLabel(groupBox);
        label->setObjectName("label");
        QSizePolicy sizePolicy2(QSizePolicy::Policy::Preferred, QSizePolicy::Policy::Preferred);
        sizePolicy2.setHorizontalStretch(0);
        sizePolicy2.setVerticalStretch(0);
        sizePolicy2.setHeightForWidth(label->sizePolicy().hasHeightForWidth());
        label->setSizePolicy(sizePolicy2);
        label->setMinimumSize(QSize(0, 30));

        formLayout->setWidget(0, QFormLayout::ItemRole::LabelRole, label);

        periodSpinBox = new QDoubleSpinBox(groupBox);
        periodSpinBox->setObjectName("periodSpinBox");
        periodSpinBox->setEnabled(false);
        QSizePolicy sizePolicy3(QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Fixed);
        sizePolicy3.setHorizontalStretch(0);
        sizePolicy3.setVerticalStretch(0);
        sizePolicy3.setHeightForWidth(periodSpinBox->sizePolicy().hasHeightForWidth());
        periodSpinBox->setSizePolicy(sizePolicy3);
        periodSpinBox->setMinimumSize(QSize(0, 30));
        periodSpinBox->setMinimum(-1.000000000000000);
        periodSpinBox->setSingleStep(0.100000000000000);
        periodSpinBox->setValue(-1.000000000000000);

        formLayout->setWidget(0, QFormLayout::ItemRole::FieldRole, periodSpinBox);

        amplitudeSpinBox = new QDoubleSpinBox(groupBox);
        amplitudeSpinBox->setObjectName("amplitudeSpinBox");
        amplitudeSpinBox->setEnabled(false);
        amplitudeSpinBox->setMinimumSize(QSize(0, 30));
        amplitudeSpinBox->setMinimum(-1.000000000000000);
        amplitudeSpinBox->setSingleStep(0.100000000000000);
        amplitudeSpinBox->setValue(-1.000000000000000);

        formLayout->setWidget(2, QFormLayout::ItemRole::FieldRole, amplitudeSpinBox);

        label_3 = new QLabel(groupBox);
        label_3->setObjectName("label_3");
        label_3->setMinimumSize(QSize(0, 30));

        formLayout->setWidget(4, QFormLayout::ItemRole::LabelRole, label_3);

        overshootSpinBox = new QDoubleSpinBox(groupBox);
        overshootSpinBox->setObjectName("overshootSpinBox");
        overshootSpinBox->setEnabled(false);
        overshootSpinBox->setMinimumSize(QSize(0, 30));
        overshootSpinBox->setMinimum(-1.000000000000000);
        overshootSpinBox->setSingleStep(0.100000000000000);
        overshootSpinBox->setValue(-1.000000000000000);

        formLayout->setWidget(4, QFormLayout::ItemRole::FieldRole, overshootSpinBox);

        label_2 = new QLabel(groupBox);
        label_2->setObjectName("label_2");
        label_2->setMinimumSize(QSize(0, 30));

        formLayout->setWidget(2, QFormLayout::ItemRole::LabelRole, label_2);


        verticalLayout->addWidget(groupBox);

        verticalSpacer = new QSpacerItem(20, 40, QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Expanding);

        verticalLayout->addItem(verticalSpacer);


        gridLayout->addLayout(verticalLayout, 1, 0, 1, 1);

        graphicsView = new QGraphicsView(Form);
        graphicsView->setObjectName("graphicsView");
        QSizePolicy sizePolicy4(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Expanding);
        sizePolicy4.setHorizontalStretch(0);
        sizePolicy4.setVerticalStretch(0);
        sizePolicy4.setHeightForWidth(graphicsView->sizePolicy().hasHeightForWidth());
        graphicsView->setSizePolicy(sizePolicy4);

        gridLayout->addWidget(graphicsView, 1, 1, 1, 1);


        retranslateUi(Form);

        QMetaObject::connectSlotsByName(Form);
    } // setupUi

    void retranslateUi(QWidget *Form)
    {
        Form->setWindowTitle(QCoreApplication::translate("Form", "Easing curves", nullptr));
        groupBox_2->setTitle(QCoreApplication::translate("Form", "Path type", nullptr));
        lineRadio->setText(QCoreApplication::translate("Form", "Line", nullptr));
        circleRadio->setText(QCoreApplication::translate("Form", "Circle", nullptr));
        groupBox->setTitle(QCoreApplication::translate("Form", "Properties", nullptr));
        label->setText(QCoreApplication::translate("Form", "Period", nullptr));
        label_3->setText(QCoreApplication::translate("Form", "Overshoot", nullptr));
        label_2->setText(QCoreApplication::translate("Form", "Amplitude", nullptr));
    } // retranslateUi

};

namespace Ui {
    class Form: public Ui_Form {};
} // namespace Ui

QT_END_NAMESPACE

#endif // BUTTONGROUP_H

/********************************************************************************
** Form generated from reading UI file 'buttongroup.ui'
**
** Created by: Qt User Interface Compiler version 5.10.1
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
            Form->setObjectName(QStringLiteral("Form"));
        Form->resize(545, 471);
        gridLayout = new QGridLayout(Form);
        gridLayout->setObjectName(QStringLiteral("gridLayout"));
        easingCurvePicker = new QListWidget(Form);
        easingCurvePicker->setObjectName(QStringLiteral("easingCurvePicker"));
        QSizePolicy sizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
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
        verticalLayout->setObjectName(QStringLiteral("verticalLayout"));
        groupBox_2 = new QGroupBox(Form);
        groupBox_2->setObjectName(QStringLiteral("groupBox_2"));
        groupBox_2->setMaximumSize(QSize(16777215, 16777215));
        gridLayout_2 = new QGridLayout(groupBox_2);
        gridLayout_2->setObjectName(QStringLiteral("gridLayout_2"));
        lineRadio = new QRadioButton(groupBox_2);
        buttonGroup = new QButtonGroup(Form);
        buttonGroup->setObjectName(QStringLiteral("buttonGroup"));
        buttonGroup->addButton(lineRadio);
        lineRadio->setObjectName(QStringLiteral("lineRadio"));
        lineRadio->setMaximumSize(QSize(16777215, 40));
        lineRadio->setLayoutDirection(Qt::LeftToRight);
        lineRadio->setChecked(true);

        gridLayout_2->addWidget(lineRadio, 0, 0, 1, 1);

        circleRadio = new QRadioButton(groupBox_2);
        buttonGroup->addButton(circleRadio);
        circleRadio->setObjectName(QStringLiteral("circleRadio"));
        circleRadio->setMaximumSize(QSize(16777215, 40));

        gridLayout_2->addWidget(circleRadio, 1, 0, 1, 1);


        verticalLayout->addWidget(groupBox_2);

        groupBox = new QGroupBox(Form);
        groupBox->setObjectName(QStringLiteral("groupBox"));
        QSizePolicy sizePolicy1(QSizePolicy::Fixed, QSizePolicy::Preferred);
        sizePolicy1.setHorizontalStretch(0);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(groupBox->sizePolicy().hasHeightForWidth());
        groupBox->setSizePolicy(sizePolicy1);
        formLayout = new QFormLayout(groupBox);
        formLayout->setObjectName(QStringLiteral("formLayout"));
        formLayout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
        label = new QLabel(groupBox);
        label->setObjectName(QStringLiteral("label"));
        QSizePolicy sizePolicy2(QSizePolicy::Preferred, QSizePolicy::Preferred);
        sizePolicy2.setHorizontalStretch(0);
        sizePolicy2.setVerticalStretch(0);
        sizePolicy2.setHeightForWidth(label->sizePolicy().hasHeightForWidth());
        label->setSizePolicy(sizePolicy2);
        label->setMinimumSize(QSize(0, 30));

        formLayout->setWidget(0, QFormLayout::LabelRole, label);

        periodSpinBox = new QDoubleSpinBox(groupBox);
        periodSpinBox->setObjectName(QStringLiteral("periodSpinBox"));
        periodSpinBox->setEnabled(false);
        QSizePolicy sizePolicy3(QSizePolicy::Minimum, QSizePolicy::Fixed);
        sizePolicy3.setHorizontalStretch(0);
        sizePolicy3.setVerticalStretch(0);
        sizePolicy3.setHeightForWidth(periodSpinBox->sizePolicy().hasHeightForWidth());
        periodSpinBox->setSizePolicy(sizePolicy3);
        periodSpinBox->setMinimumSize(QSize(0, 30));
        periodSpinBox->setMinimum(-1);
        periodSpinBox->setSingleStep(0.1);
        periodSpinBox->setValue(-1);

        formLayout->setWidget(0, QFormLayout::FieldRole, periodSpinBox);

        amplitudeSpinBox = new QDoubleSpinBox(groupBox);
        amplitudeSpinBox->setObjectName(QStringLiteral("amplitudeSpinBox"));
        amplitudeSpinBox->setEnabled(false);
        amplitudeSpinBox->setMinimumSize(QSize(0, 30));
        amplitudeSpinBox->setMinimum(-1);
        amplitudeSpinBox->setSingleStep(0.1);
        amplitudeSpinBox->setValue(-1);

        formLayout->setWidget(2, QFormLayout::FieldRole, amplitudeSpinBox);

        label_3 = new QLabel(groupBox);
        label_3->setObjectName(QStringLiteral("label_3"));
        label_3->setMinimumSize(QSize(0, 30));

        formLayout->setWidget(4, QFormLayout::LabelRole, label_3);

        overshootSpinBox = new QDoubleSpinBox(groupBox);
        overshootSpinBox->setObjectName(QStringLiteral("overshootSpinBox"));
        overshootSpinBox->setEnabled(false);
        overshootSpinBox->setMinimumSize(QSize(0, 30));
        overshootSpinBox->setMinimum(-1);
        overshootSpinBox->setSingleStep(0.1);
        overshootSpinBox->setValue(-1);

        formLayout->setWidget(4, QFormLayout::FieldRole, overshootSpinBox);

        label_2 = new QLabel(groupBox);
        label_2->setObjectName(QStringLiteral("label_2"));
        label_2->setMinimumSize(QSize(0, 30));

        formLayout->setWidget(2, QFormLayout::LabelRole, label_2);


        verticalLayout->addWidget(groupBox);

        verticalSpacer = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

        verticalLayout->addItem(verticalSpacer);


        gridLayout->addLayout(verticalLayout, 1, 0, 1, 1);

        graphicsView = new QGraphicsView(Form);
        graphicsView->setObjectName(QStringLiteral("graphicsView"));
        QSizePolicy sizePolicy4(QSizePolicy::Expanding, QSizePolicy::Expanding);
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
        Form->setWindowTitle(QApplication::translate("Form", "Easing curves", nullptr));
        groupBox_2->setTitle(QApplication::translate("Form", "Path type", nullptr));
        lineRadio->setText(QApplication::translate("Form", "Line", nullptr));
        circleRadio->setText(QApplication::translate("Form", "Circle", nullptr));
        groupBox->setTitle(QApplication::translate("Form", "Properties", nullptr));
        label->setText(QApplication::translate("Form", "Period", nullptr));
        label_3->setText(QApplication::translate("Form", "Overshoot", nullptr));
        label_2->setText(QApplication::translate("Form", "Amplitude", nullptr));
    } // retranslateUi

};

namespace Ui {
    class Form: public Ui_Form {};
} // namespace Ui

QT_END_NAMESPACE

#endif // BUTTONGROUP_H

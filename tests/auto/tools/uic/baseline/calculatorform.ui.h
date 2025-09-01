/********************************************************************************
** Form generated from reading UI file 'calculatorform.ui'
**
** Created by: Qt User Interface Compiler version 6.0.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef CALCULATORFORM_H
#define CALCULATORFORM_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_CalculatorForm
{
public:
    QGridLayout *gridLayout;
    QHBoxLayout *hboxLayout;
    QVBoxLayout *vboxLayout;
    QLabel *label;
    QSpinBox *inputSpinBox1;
    QLabel *label_3;
    QVBoxLayout *vboxLayout1;
    QLabel *label_2;
    QSpinBox *inputSpinBox2;
    QLabel *label_3_2;
    QVBoxLayout *vboxLayout2;
    QLabel *label_2_2_2;
    QLabel *outputWidget;
    QSpacerItem *verticalSpacer;
    QSpacerItem *horizontalSpacer;

    void setupUi(QWidget *CalculatorForm)
    {
        if (CalculatorForm->objectName().isEmpty())
            CalculatorForm->setObjectName("CalculatorForm");
        CalculatorForm->resize(276, 98);
        QSizePolicy sizePolicy(QSizePolicy::Policy::Preferred, QSizePolicy::Policy::Preferred);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(CalculatorForm->sizePolicy().hasHeightForWidth());
        CalculatorForm->setSizePolicy(sizePolicy);
        gridLayout = new QGridLayout(CalculatorForm);
#ifndef Q_OS_MAC
        gridLayout->setSpacing(6);
#endif
#ifndef Q_OS_MAC
        gridLayout->setContentsMargins(9, 9, 9, 9);
#endif
        gridLayout->setObjectName("gridLayout");
        hboxLayout = new QHBoxLayout();
#ifndef Q_OS_MAC
        hboxLayout->setSpacing(6);
#endif
        hboxLayout->setContentsMargins(1, 1, 1, 1);
        hboxLayout->setObjectName("hboxLayout");
        vboxLayout = new QVBoxLayout();
#ifndef Q_OS_MAC
        vboxLayout->setSpacing(6);
#endif
        vboxLayout->setContentsMargins(1, 1, 1, 1);
        vboxLayout->setObjectName("vboxLayout");
        label = new QLabel(CalculatorForm);
        label->setObjectName("label");

        vboxLayout->addWidget(label);

        inputSpinBox1 = new QSpinBox(CalculatorForm);
        inputSpinBox1->setObjectName("inputSpinBox1");
        inputSpinBox1->setMouseTracking(true);

        vboxLayout->addWidget(inputSpinBox1);


        hboxLayout->addLayout(vboxLayout);

        label_3 = new QLabel(CalculatorForm);
        label_3->setObjectName("label_3");
        label_3->setAlignment(Qt::AlignmentFlag::AlignCenter);

        hboxLayout->addWidget(label_3);

        vboxLayout1 = new QVBoxLayout();
#ifndef Q_OS_MAC
        vboxLayout1->setSpacing(6);
#endif
        vboxLayout1->setContentsMargins(1, 1, 1, 1);
        vboxLayout1->setObjectName("vboxLayout1");
        label_2 = new QLabel(CalculatorForm);
        label_2->setObjectName("label_2");

        vboxLayout1->addWidget(label_2);

        inputSpinBox2 = new QSpinBox(CalculatorForm);
        inputSpinBox2->setObjectName("inputSpinBox2");
        inputSpinBox2->setMouseTracking(true);

        vboxLayout1->addWidget(inputSpinBox2);


        hboxLayout->addLayout(vboxLayout1);

        label_3_2 = new QLabel(CalculatorForm);
        label_3_2->setObjectName("label_3_2");
        label_3_2->setAlignment(Qt::AlignmentFlag::AlignCenter);

        hboxLayout->addWidget(label_3_2);

        vboxLayout2 = new QVBoxLayout();
#ifndef Q_OS_MAC
        vboxLayout2->setSpacing(6);
#endif
        vboxLayout2->setContentsMargins(1, 1, 1, 1);
        vboxLayout2->setObjectName("vboxLayout2");
        label_2_2_2 = new QLabel(CalculatorForm);
        label_2_2_2->setObjectName("label_2_2_2");

        vboxLayout2->addWidget(label_2_2_2);

        outputWidget = new QLabel(CalculatorForm);
        outputWidget->setObjectName("outputWidget");
        outputWidget->setFrameShape(QFrame::Shape::Box);
        outputWidget->setFrameShadow(QFrame::Shadow::Sunken);
        outputWidget->setAlignment(Qt::AlignmentFlag::AlignAbsolute|Qt::AlignmentFlag::AlignBaseline|Qt::AlignmentFlag::AlignBottom|Qt::AlignmentFlag::AlignCenter|Qt::AlignmentFlag::AlignHCenter|Qt::AlignmentFlag::AlignHorizontal_Mask|Qt::AlignmentFlag::AlignJustify|Qt::AlignmentFlag::AlignLeading|Qt::AlignmentFlag::AlignLeft|Qt::AlignmentFlag::AlignRight|Qt::AlignmentFlag::AlignTop|Qt::AlignmentFlag::AlignTrailing|Qt::AlignmentFlag::AlignVCenter|Qt::AlignmentFlag::AlignVertical_Mask);

        vboxLayout2->addWidget(outputWidget);


        hboxLayout->addLayout(vboxLayout2);


        gridLayout->addLayout(hboxLayout, 0, 0, 1, 1);

        verticalSpacer = new QSpacerItem(20, 40, QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Expanding);

        gridLayout->addItem(verticalSpacer, 1, 0, 1, 1);

        horizontalSpacer = new QSpacerItem(40, 20, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        gridLayout->addItem(horizontalSpacer, 0, 1, 1, 1);


        retranslateUi(CalculatorForm);

        QMetaObject::connectSlotsByName(CalculatorForm);
    } // setupUi

    void retranslateUi(QWidget *CalculatorForm)
    {
        CalculatorForm->setWindowTitle(QCoreApplication::translate("CalculatorForm", "Calculator Builder", nullptr));
        label->setText(QCoreApplication::translate("CalculatorForm", "Input 1", nullptr));
        label_3->setText(QCoreApplication::translate("CalculatorForm", "+", nullptr));
        label_2->setText(QCoreApplication::translate("CalculatorForm", "Input 2", nullptr));
        label_3_2->setText(QCoreApplication::translate("CalculatorForm", "=", nullptr));
        label_2_2_2->setText(QCoreApplication::translate("CalculatorForm", "Output", nullptr));
        outputWidget->setText(QCoreApplication::translate("CalculatorForm", "0", nullptr));
    } // retranslateUi

};

namespace Ui {
    class CalculatorForm: public Ui_CalculatorForm {};
} // namespace Ui

QT_END_NAMESPACE

#endif // CALCULATORFORM_H

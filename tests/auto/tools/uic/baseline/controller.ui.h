/********************************************************************************
** Form generated from reading UI file 'controller.ui'
**
** Created by: Qt User Interface Compiler version 5.0.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef CONTROLLER_H
#define CONTROLLER_H

#include <QtCore/QVariant>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_Controller
{
public:
    QGridLayout *gridLayout;
    QLabel *label;
    QPushButton *decelerate;
    QPushButton *accelerate;
    QPushButton *right;
    QPushButton *left;

    void setupUi(QWidget *Controller)
    {
        if (Controller->objectName().isEmpty())
            Controller->setObjectName(QStringLiteral("Controller"));
        Controller->resize(255, 111);
        gridLayout = new QGridLayout(Controller);
#ifndef Q_OS_MAC
        gridLayout->setSpacing(6);
#endif
#ifndef Q_OS_MAC
        gridLayout->setContentsMargins(9, 9, 9, 9);
#endif
        gridLayout->setObjectName(QStringLiteral("gridLayout"));
        label = new QLabel(Controller);
        label->setObjectName(QStringLiteral("label"));
        label->setAlignment(Qt::AlignCenter);

        gridLayout->addWidget(label, 1, 1, 1, 1);

        decelerate = new QPushButton(Controller);
        decelerate->setObjectName(QStringLiteral("decelerate"));

        gridLayout->addWidget(decelerate, 2, 1, 1, 1);

        accelerate = new QPushButton(Controller);
        accelerate->setObjectName(QStringLiteral("accelerate"));

        gridLayout->addWidget(accelerate, 0, 1, 1, 1);

        right = new QPushButton(Controller);
        right->setObjectName(QStringLiteral("right"));

        gridLayout->addWidget(right, 1, 2, 1, 1);

        left = new QPushButton(Controller);
        left->setObjectName(QStringLiteral("left"));

        gridLayout->addWidget(left, 1, 0, 1, 1);


        retranslateUi(Controller);

        QMetaObject::connectSlotsByName(Controller);
    } // setupUi

    void retranslateUi(QWidget *Controller)
    {
        Controller->setWindowTitle(QApplication::translate("Controller", "Controller", 0));
        label->setText(QApplication::translate("Controller", "Controller", 0));
        decelerate->setText(QApplication::translate("Controller", "Decelerate", 0));
        accelerate->setText(QApplication::translate("Controller", "Accelerate", 0));
        right->setText(QApplication::translate("Controller", "Right", 0));
        left->setText(QApplication::translate("Controller", "Left", 0));
    } // retranslateUi

};

namespace Ui {
    class Controller: public Ui_Controller {};
} // namespace Ui

QT_END_NAMESPACE

#endif // CONTROLLER_H

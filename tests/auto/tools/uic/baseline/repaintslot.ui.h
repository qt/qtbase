/********************************************************************************
** Form generated from reading UI file 'repaintslot.ui'
**
** Created by: Qt User Interface Compiler version 6.0.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef REPAINTSLOT_H
#define REPAINTSLOT_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QSlider>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_Widget
{
public:
    QVBoxLayout *verticalLayout;
    QSlider *verticalSlider;

    void setupUi(QWidget *Widget)
    {
        if (Widget->objectName().isEmpty())
            Widget->setObjectName("Widget");
        Widget->resize(800, 600);
        verticalLayout = new QVBoxLayout(Widget);
        verticalLayout->setObjectName("verticalLayout");
        verticalSlider = new QSlider(Widget);
        verticalSlider->setObjectName("verticalSlider");
        verticalSlider->setOrientation(Qt::Vertical);

        verticalLayout->addWidget(verticalSlider);


        retranslateUi(Widget);
        QObject::connect(verticalSlider, &QSlider::valueChanged, Widget, qOverload<>(&QWidget::repaint));

        QMetaObject::connectSlotsByName(Widget);
    } // setupUi

    void retranslateUi(QWidget *Widget)
    {
        Widget->setWindowTitle(QCoreApplication::translate("Widget", "Widget", nullptr));
    } // retranslateUi

};

namespace Ui {
    class Widget: public Ui_Widget {};
} // namespace Ui

QT_END_NAMESPACE

#endif // REPAINTSLOT_H

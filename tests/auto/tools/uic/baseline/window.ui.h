/********************************************************************************
** Form generated from reading UI file 'window.ui'
**
** Created by: Qt User Interface Compiler version 6.0.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef WINDOW_H
#define WINDOW_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QToolButton>
#include <QtWidgets/QWidget>
#include "displaywidget.h"

QT_BEGIN_NAMESPACE

class Ui_Window
{
public:
    QGridLayout *gridLayout_2;
    QSpacerItem *horizontalSpacer_2;
    DisplayWidget *displayWidget;
    QSpacerItem *horizontalSpacer_3;
    QGridLayout *gridLayout;
    QLabel *label;
    QComboBox *shapeComboBox;
    QLabel *label_2;
    QToolButton *colorButton;
    QLabel *label_3;
    QComboBox *shapeComboBox_2;
    QHBoxLayout *horizontalLayout;
    QSpacerItem *horizontalSpacer;
    QToolButton *toolButton_2;

    void setupUi(QWidget *Window)
    {
        if (Window->objectName().isEmpty())
            Window->setObjectName("Window");
        Window->resize(339, 353);
        gridLayout_2 = new QGridLayout(Window);
        gridLayout_2->setObjectName("gridLayout_2");
        gridLayout_2->setSizeConstraint(QLayout::SetFixedSize);
        horizontalSpacer_2 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        gridLayout_2->addItem(horizontalSpacer_2, 0, 0, 1, 1);

        displayWidget = new DisplayWidget(Window);
        displayWidget->setObjectName("displayWidget");
        QSizePolicy sizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        sizePolicy.setHorizontalStretch(200);
        sizePolicy.setVerticalStretch(200);
        sizePolicy.setHeightForWidth(displayWidget->sizePolicy().hasHeightForWidth());
        displayWidget->setSizePolicy(sizePolicy);
        displayWidget->setMinimumSize(QSize(200, 200));
        displayWidget->setMaximumSize(QSize(200, 200));

        gridLayout_2->addWidget(displayWidget, 0, 1, 1, 1);

        horizontalSpacer_3 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        gridLayout_2->addItem(horizontalSpacer_3, 0, 2, 1, 1);

        gridLayout = new QGridLayout();
        gridLayout->setObjectName("gridLayout");
        label = new QLabel(Window);
        label->setObjectName("label");

        gridLayout->addWidget(label, 0, 0, 1, 1);

        shapeComboBox = new QComboBox(Window);
        shapeComboBox->addItem(QString());
        shapeComboBox->addItem(QString());
        shapeComboBox->setObjectName("shapeComboBox");

        gridLayout->addWidget(shapeComboBox, 0, 1, 1, 1);

        label_2 = new QLabel(Window);
        label_2->setObjectName("label_2");

        gridLayout->addWidget(label_2, 1, 0, 1, 1);

        colorButton = new QToolButton(Window);
        colorButton->setObjectName("colorButton");

        gridLayout->addWidget(colorButton, 1, 1, 1, 1);

        label_3 = new QLabel(Window);
        label_3->setObjectName("label_3");

        gridLayout->addWidget(label_3, 2, 0, 1, 1);

        shapeComboBox_2 = new QComboBox(Window);
        shapeComboBox_2->addItem(QString());
        shapeComboBox_2->addItem(QString());
        shapeComboBox_2->addItem(QString());
        shapeComboBox_2->setObjectName("shapeComboBox_2");

        gridLayout->addWidget(shapeComboBox_2, 2, 1, 1, 1);


        gridLayout_2->addLayout(gridLayout, 1, 0, 1, 3);

        horizontalLayout = new QHBoxLayout();
        horizontalLayout->setObjectName("horizontalLayout");
        horizontalSpacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout->addItem(horizontalSpacer);

        toolButton_2 = new QToolButton(Window);
        toolButton_2->setObjectName("toolButton_2");

        horizontalLayout->addWidget(toolButton_2);


        gridLayout_2->addLayout(horizontalLayout, 2, 0, 1, 3);

#if QT_CONFIG(shortcut)
        label->setBuddy(shapeComboBox);
        label_2->setBuddy(colorButton);
        label_3->setBuddy(shapeComboBox_2);
#endif // QT_CONFIG(shortcut)

        retranslateUi(Window);
        QObject::connect(shapeComboBox, SIGNAL(currentIndexChanged(int)), Window, SLOT(updateShape(int)));
        QObject::connect(colorButton, SIGNAL(clicked()), Window, SLOT(updateColor()));
        QObject::connect(shapeComboBox_2, SIGNAL(currentIndexChanged(int)), Window, SLOT(updateBackground(int)));
        QObject::connect(toolButton_2, SIGNAL(clicked()), Window, SLOT(saveSvg()));

        QMetaObject::connectSlotsByName(Window);
    } // setupUi

    void retranslateUi(QWidget *Window)
    {
        Window->setWindowTitle(QCoreApplication::translate("Window", "SVG Generator", nullptr));
        label->setText(QCoreApplication::translate("Window", "&Shape:", nullptr));
        shapeComboBox->setItemText(0, QCoreApplication::translate("Window", "House", nullptr));
        shapeComboBox->setItemText(1, QCoreApplication::translate("Window", "Car", nullptr));

        label_2->setText(QCoreApplication::translate("Window", "&Color:", nullptr));
        colorButton->setText(QCoreApplication::translate("Window", "Choose Color...", nullptr));
        label_3->setText(QCoreApplication::translate("Window", "&Background:", nullptr));
        shapeComboBox_2->setItemText(0, QCoreApplication::translate("Window", "Sky", nullptr));
        shapeComboBox_2->setItemText(1, QCoreApplication::translate("Window", "Trees", nullptr));
        shapeComboBox_2->setItemText(2, QCoreApplication::translate("Window", "Road", nullptr));

        toolButton_2->setText(QCoreApplication::translate("Window", "Save &As...", nullptr));
    } // retranslateUi

};

namespace Ui {
    class Window: public Ui_Window {};
} // namespace Ui

QT_END_NAMESPACE

#endif // WINDOW_H
